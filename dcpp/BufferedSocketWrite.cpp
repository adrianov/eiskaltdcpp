/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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

#include <algorithm>

#include "Streams.h"
#include "ThrottleManager.h"

namespace dcpp {

using std::min;
using std::max;

#define POLL_TIMEOUT 250

void BufferedSocket::threadSendFile(InputStream* file) {
    if(state != RUNNING)
        return;

    if(disconnecting)
        return;
    dcassert(file != NULL);
    size_t sockSize = (size_t)sock->getSocketOptInt(SO_SNDBUF);
    size_t bufSize = max(sockSize, (size_t)64*1024);

    ByteVector readBuf(bufSize);
    ByteVector writeBuf(bufSize);

    size_t readPos = 0;

    bool readDone = false;
    dcdebug("Starting threadSend\n");
    while(!disconnecting) {
        if(!readDone && readBuf.size() > readPos) {
            size_t bytesRead = readBuf.size() - readPos;
            size_t actual = file->read(&readBuf[readPos], bytesRead);

            if(bytesRead > 0) {
                fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
            }

            if(actual == 0) {
                readDone = true;
            } else {
                readPos += actual;
            }
        }

        if(readDone && readPos == 0) {
            fire(BufferedSocketListener::TransmitDone());
            return;
        }

        readBuf.swap(writeBuf);
        readBuf.resize(bufSize);
        writeBuf.resize(readPos);
        readPos = 0;

        size_t writePos = 0, writeSize = 0;
        int written = 0;

        while(writePos < writeBuf.size()) {
            if(disconnecting)
                return;

            int w = sock->wait(0, Socket::WAIT_READ);
            if(w & Socket::WAIT_READ) {
                threadRead();
            }

            if(written == -1) {
                // OpenSSL: retry after failed write must use same size.
                try {
                    written = sock->write(&writeBuf[writePos], writeSize);
                } catch(const Exception&) {
                    // ...
                }
            } else {
                writeSize = min(sockSize / 2, writeBuf.size() - writePos);
                written = ThrottleManager::getInstance()->write(sock.get(), &writeBuf[writePos], writeSize);
            }

            if(written > 0) {
                writePos += written;

                fire(BufferedSocketListener::BytesSent(), 0, written);

            } else if(written == -1) {
                if(!readDone && readPos < readBuf.size()) {
                    size_t bytesRead = min(readBuf.size() - readPos, readBuf.size() / 2);
                    size_t actual = file->read(&readBuf[readPos], bytesRead);

                    if(bytesRead > 0) {
                        fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
                    }

                    if(actual == 0) {
                        readDone = true;
                    } else {
                        readPos += actual;
                    }
                } else {
                    while(!disconnecting) {
                        int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_WRITE | Socket::WAIT_READ);
                        if(w & Socket::WAIT_READ) {
                            threadRead();
                        }
                        if(w & Socket::WAIT_WRITE) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

void BufferedSocket::write(const char* aBuf, size_t aLen) noexcept {
    if(!sock.get())
        return;
    Lock l(cs);
    if(writeBuf.empty())
        addTask(SEND_DATA, 0);

    writeBuf.insert(writeBuf.end(), aBuf, aBuf+aLen);
}

void BufferedSocket::threadSendData() {
    if(state != RUNNING)
        return;

    {
        Lock l(cs);
        if(writeBuf.empty())
            return;

        writeBuf.swap(sendBuf);
    }

    size_t left = sendBuf.size();
    size_t done = 0;
    while(left > 0) {
        if(disconnecting) {
            return;
        }

        int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ | Socket::WAIT_WRITE);

        if(w & Socket::WAIT_READ) {
            threadRead();
        }

        if(w & Socket::WAIT_WRITE) {
            int n = sock->write(&sendBuf[done], left);
            if(n > 0) {
                left -= n;
                done += n;
            }
        }
    }
    sendBuf.clear();
}

} // namespace dcpp
