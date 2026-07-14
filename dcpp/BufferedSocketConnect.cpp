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

#include "CryptoManager.h"
#include "SettingsManager.h"
#include "SSLSocket.h"
#include "TimerManager.h"
#include "format.h"

namespace dcpp {

#define POLL_TIMEOUT 250
#define LONG_TIMEOUT 30000
#define SHORT_TIMEOUT 1000

void BufferedSocket::setSocket(std::unique_ptr<Socket> s) {
    dcassert(!sock.get());
    if(SETTING(SOCKET_IN_BUFFER) > 0)
        s->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
    if(SETTING(SOCKET_OUT_BUFFER) > 0)
        s->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
    s->setSocketOpt(SO_REUSEADDR, 1);   // NAT traversal

    inbuf.resize(s->getSocketOptInt(SO_RCVBUF));

    sock = std::move(s);
}

void BufferedSocket::accept(const Socket& srv, bool secure, bool allowUntrusted) {
    dcdebug("BufferedSocket::accept() %p\n", (void*)this);

    std::unique_ptr<Socket> s(secure ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : new Socket);

    s->accept(srv);

    setSocket(std::move(s));

    Lock l(cs);
    addTask(ACCEPTED, 0);
}

void BufferedSocket::connect(const string& aAddress, const string& aPort, bool secure, bool allowUntrusted, bool proxy, Socket::Protocol proto, const string& expKP) {
    connect(aAddress, aPort, Util::emptyString, NAT_NONE, secure, allowUntrusted, proxy, proto, expKP);
}

void BufferedSocket::connect(const string& aAddress, const string& aPort, const string& localPort, NatRoles natRole, bool secure, bool allowUntrusted, bool proxy, Socket::Protocol proto, const string& expKP) {
    (void)expKP;
    dcdebug("BufferedSocket::connect() %p\n", (void*)this);
    std::unique_ptr<Socket> s(secure ? (natRole == NAT_SERVER ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : CryptoManager::getInstance()->getClientSocket(allowUntrusted, proto)) : new Socket);

    s->create();
    setSocket(std::move(s));
    sock->bind(localPort, SETTING(BIND_IFACE)? sock->getIfaceI4(SETTING(BIND_IFACE_NAME)).c_str() : SETTING(BIND_ADDRESS));

    Lock l(cs);
    addTask(CONNECT, new ConnectInfo(aAddress, aPort, localPort, natRole, proxy && (SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)));
}

void BufferedSocket::threadConnect(const string& aAddr, const string &aPort, const string &localPort, NatRoles natRole, bool proxy) {
    dcassert(state == STARTING);

    dcdebug("threadConnect %s:%s/%s\n", aAddr.c_str(), localPort.c_str(), aPort.c_str());
    fire(BufferedSocketListener::Connecting());

    const uint64_t endTime = GET_TICK() + LONG_TIMEOUT;
    state = RUNNING;

    while (GET_TICK() < endTime) {
        dcdebug("threadConnect attempt to addr \"%s\"\n", aAddr.c_str());
        try {
            if(proxy) {
                sock->socksConnect(aAddr, aPort, LONG_TIMEOUT);
            } else {
                sock->connect(aAddr, aPort);
            }

            bool connSucceeded;
            while(!(connSucceeded = sock->waitConnected(POLL_TIMEOUT)) && endTime >= GET_TICK()) {
                if(disconnecting) return;
            }

            if (connSucceeded) {
                fire(BufferedSocketListener::Connected());
                return;
            }
        }
        catch (const SSLSocketException&) {
            throw;
        } catch (const SocketException&) {
            if (natRole == NAT_NONE)
                throw;
            Thread::sleep(SHORT_TIMEOUT);
        }
    }

    throw SocketException(_("Connection timeout"));
}

void BufferedSocket::threadAccept() {
    dcassert(state == STARTING);

    dcdebug("threadAccept\n");

    state = RUNNING;

    uint64_t startTime = GET_TICK();
    while(!sock->waitAccepted(POLL_TIMEOUT)) {
        if(disconnecting)
            return;

        if((startTime + 30000) < GET_TICK()) {
            throw SocketException(_("Connection timeout"));
        }
    }
}

} // namespace dcpp
