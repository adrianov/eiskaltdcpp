/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "Client.h"
#include "ClientManager.h"
#include "Thread.h"
#include "UserConnection.h"

namespace dcpp {

void ConnectionManager::disconnect(const UserPtr& user) {
    Lock l(cs);
    for(auto uc: userConnections) {
        if(uc->getUser() == user)
            uc->disconnect(true);
    }
}

void ConnectionManager::disconnect(const UserPtr& user, int isDownload) {
    Lock l(cs);
    for(auto uc: userConnections) {
        if(uc->getUser() == user && uc->isSet(isDownload ? UserConnection::FLAG_DOWNLOAD : UserConnection::FLAG_UPLOAD)) {
            uc->disconnect(true);
            break;
        }
    }
}

void ConnectionManager::blockRetry(const UserPtr& user) {
    Lock l(cs);
    auto i = find(downloads.begin(), downloads.end(), user);
    if(i != downloads.end()) {
        (*i)->setErrors(-1);
        if((*i)->getState() == ConnectionQueueItem::CONNECTING)
            (*i)->setState(ConnectionQueueItem::WAITING);
    }
    disconnect(user);
}

void ConnectionManager::shutdown() {
    // Idempotent: Qt/GTK/daemon may close peers before destroying hub widgets.
    if(shuttingDown) {
        for(int i = 0; i < 300; ++i) {
            {
                Lock l(cs);
                if(userConnections.empty())
                    return;
            }
            Thread::sleep(50);
        }
        return;
    }

    TimerManager::getInstance()->removeListener(this);
    shuttingDown = true;
    disconnect();

    {
        Lock l(cs);
        for(auto j: userConnections)
            j->disconnect(false);
    }

    // Let uploads finish the current send / TLS close without blocking quit forever.
    const uint64_t graceUntil = GET_TICK() + 3000;
    while(GET_TICK() < graceUntil) {
        {
            Lock l(cs);
            if(userConnections.empty())
                break;
        }
        Thread::sleep(50);
    }

    {
        Lock l(cs);
        for(auto j: userConnections)
            j->disconnect(true);
    }
    for(int i = 0; i < 300; ++i) {
        {
            Lock l(cs);
            if(userConnections.empty())
                break;
        }
        Thread::sleep(50);
    }

    // Leave hubs after peers so upload grace is not cut short by offline kicks.
    // Client::shutdown (putSocket) — disconnect alone leaves the socket thread waiting for SHUTDOWN.
    {
        const Client::List hubs = ClientManager::getInstance()->getClients();
        for(auto c : hubs) {
            c->setAutoReconnect(false);
            c->shutdown();
        }
    }

    // Brief pause so TCP/TLS close can leave the host before process teardown.
    Thread::sleep(250);
}

void ConnectionManager::on(UserConnectionListener::Supports, UserConnection* conn, const StringList& feat) noexcept {
    for(auto& i: feat) {
        if(i == UserConnection::FEATURE_MINISLOTS) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
        } else if(i == UserConnection::FEATURE_XML_BZLIST) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
        } else if(i == UserConnection::FEATURE_ADCGET) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
        } else if(i == UserConnection::FEATURE_ZLIB_GET) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
        } else if(i == UserConnection::FEATURE_TTHL) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
        } else if(i == UserConnection::FEATURE_TTHF) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
        }
    }
}

} // namespace dcpp
