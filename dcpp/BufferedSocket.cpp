/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"

#include "BufferedSocket.h"

#include "ZUtils.h"
#include "format.h"

namespace dcpp {

// Polling is used for tasks...should be fixed...
#define POLL_TIMEOUT 250

BufferedSocket::BufferedSocket(char aSeparator) :
    separator(aSeparator), mode(MODE_LINE), dataBytes(0), rollback(0), state(STARTING),
    disconnecting(false)
{
    start();

    sockets.inc();
}

Atomic<long,memory_ordering_strong> BufferedSocket::sockets(0);

BufferedSocket::~BufferedSocket() {
    sockets.dec();
}

void BufferedSocket::setMode (Modes aMode, size_t aRollback) {
    if (mode == aMode) {
        dcdebug ("WARNING: Re-entering mode %d\n", mode);
        return;
    }

    switch (aMode) {
    case MODE_LINE:
        rollback = aRollback;
        break;
    case MODE_ZPIPE:
        filterIn = std::unique_ptr<UnZFilter>(new UnZFilter);
        break;
    case MODE_DATA:
        break;
    }
    mode = aMode;
}

bool BufferedSocket::checkEvents() {
    while(state == RUNNING ? taskSem.wait(0) : taskSem.wait()) {
        pair<Tasks, unique_ptr<TaskData> > p;
        {
            Lock l(cs);
            dcassert(!tasks.empty());
            p = std::move(tasks.front());
            tasks.erase(tasks.begin());
        }

        if(p.first == SHUTDOWN) {
            return false;
        } else if(p.first == UPDATED) {
            fire(BufferedSocketListener::Updated());
            continue;
        }

        if(state == STARTING) {
            if(p.first == CONNECT) {
                ConnectInfo* ci = static_cast<ConnectInfo*>(p.second.get());
                threadConnect(ci->addr, ci->port, ci->localPort, ci->natRole, ci->proxy);
            } else if(p.first == ACCEPTED) {
                threadAccept();
            } else {
                dcdebug("%d unexpected in STARTING state\n", p.first);
            }
        } else if(state == RUNNING) {
            if(p.first == SEND_DATA) {
                threadSendData();
            } else if(p.first == SEND_FILE) {
                threadSendFile(static_cast<SendFileInfo*>(p.second.get())->stream); break;
            } else if(p.first == DISCONNECT) {
                fail(_("Disconnected"));
            } else {
                dcdebug("%d unexpected in RUNNING state\n", p.first);
            }
        }
    }
    return true;
}

void BufferedSocket::checkSocket() {
    if(disconnecting)
        return;

    int waitFor = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ);

    if(waitFor & Socket::WAIT_READ) {
        threadRead();
    }
}

/**
 * Main task dispatcher for the buffered socket abstraction.
 * @todo Fix the polling...
 */
int BufferedSocket::run() {
    dcdebug("BufferedSocket::run() start %p\n", (void*)this);
    while(true) {
        try {
            if(!checkEvents()) {
                break;
            }
            if(state == RUNNING) {
                if(disconnecting) {
                    // Avoid spinning: wait for DISCONNECT/SHUTDOWN instead of polling the socket.
                    taskSem.wait(POLL_TIMEOUT);
                } else {
                    checkSocket();
                }
            }
        } catch(const Exception& e) {
            fail(e.getError());
        }
    }
    dcdebug("BufferedSocket::run() end %p\n", (void*)this);
    delete this;
    return 0;
}

void BufferedSocket::fail(const string& aError) {
    if(sock.get()) {
        sock->disconnect();
    }

    if(state == RUNNING) {
        state = FAILED;
        fire(BufferedSocketListener::Failed(), aError);
    }
}

void BufferedSocket::shutdown() {
    Lock l(cs);
    disconnecting = true;
    // Unblock sock->wait so the worker can drain SHUTDOWN promptly.
    if(sock.get()) {
        sock->disconnect();
    }
    addTask(SHUTDOWN, 0);
}

void BufferedSocket::addTask(Tasks task, TaskData* data) {
    dcassert(task == DISCONNECT || task == SHUTDOWN || sock.get());
    tasks.emplace_back(task, unique_ptr<TaskData>(data)); taskSem.signal();
}

void BufferedSocket::disconnect(bool graceless) noexcept {
    Lock l(cs);
    if(graceless) {
        disconnecting = true;
        if(sock.get())
            sock->disconnect();
    }
    addTask(DISCONNECT, 0);
}

} // namespace dcpp
