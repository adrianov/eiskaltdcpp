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

#include "LogManager.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

static const uint32_t FLOOD_TRIGGER = 20000;
static const uint32_t FLOOD_ADD = 2000;
static const uint32_t POLL_TIMEOUT = 250;

ConnectionManager::Server::Server(const bool secure_, const string& aPort, const string& ip_ /* = "0.0.0.0" */) : secure(secure_), die(false) {
    sock.create();
    sock.setSocketOpt(SO_REUSEADDR, 1);
    ip = SETTING(BIND_IFACE)? sock.getIfaceI4(SETTING(BIND_IFACE_NAME)).c_str() : ip_;
    port = sock.bind(aPort, ip);
    sock.listen();

    start();
}

int ConnectionManager::Server::run() noexcept {
    {
        char threadName[17];
        snprintf(threadName, sizeof threadName, "Server_%s", port.c_str());
    }
    while(!die) {
        try {
            while(!die) {
                if(sock.wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ) {
                    ConnectionManager::getInstance()->accept(sock, secure);
                }
            }
        } catch(const Exception& e) {
            dcdebug("ConnectionManager::Server::run Error: %s\n", e.getError().c_str());
        }

        bool failed = false;
        while(!die) {
            try {
                sock.disconnect();
                sock.create();
                sock.bind(port, ip);
                sock.listen();
                if(failed) {
                    LogManager::getInstance()->message(_("Connectivity restored"));
                    failed = false;
                }
                break;
            } catch(const SocketException& e) {
                dcdebug("ConnectionManager::Server::run Stopped listening: %s\n", e.getError().c_str());

                if(!failed) {
                    LogManager::getInstance()->message(str(F_("Connectivity error: %1%") % e.getError()));
                    failed = true;
                }

                for(auto i = 0; i < 60 && !die; ++i) {
                    Thread::sleep(1000);
                }
            }
        }
    }
    return 0;
}

void ConnectionManager::accept(const Socket& sock, bool secure) noexcept {
    uint64_t now = GET_TICK();

    if(now > floodCounter) {
        floodCounter = now + FLOOD_ADD;
    } else {
        if(false && now + FLOOD_TRIGGER < floodCounter) {
            Socket s;
            try {
                s.accept(sock);
            } catch(const SocketException&) {
            }
            dcdebug("Connection flood detected!\n");
            return;
        } else {
            floodCounter += FLOOD_ADD;
        }
    }
    UserConnection* uc = getConnection(false, secure);
    uc->setFlag(UserConnection::FLAG_INCOMING);
    uc->setState(UserConnection::STATE_SUPNICK);
    uc->setLastActivity(GET_TICK());
    try {
        uc->accept(sock);
    } catch(const Exception&) {
        putConnection(uc);
    }
}

} // namespace dcpp
