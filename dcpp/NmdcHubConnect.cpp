/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "NmdcHub.h"

#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "PeerConnectLog.h"
#include "PeerConnectTls.h"
#include "format.h"

namespace dcpp {

void NmdcHub::onConnectToMe(const string& param) {
    if(state != STATE_NORMAL) {
        return;
    }
    string::size_type i = param.find(' ');
    string::size_type j;
    if( (i == string::npos) || ((i + 1) >= param.size()) ) {
        return;
    }
    i++;
    j = param.find(':', i);
    if(j == string::npos) {
        return;
    }
    string server = param.substr(i, j-i);
    if(j+1 >= param.size()) {
        return;
    }
    string senderNick;
    string port;

    i = param.find(' ', j+1);
    if(i == string::npos) {
        port = param.substr(j+1);
    } else {
        senderNick = param.substr(i+1);
        port = param.substr(j+1, i-j-1);
    }

    if(port.empty()) {
        PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$ConnectToMe ignored: empty port")));
        return;
    }

    bool secure = false;
    if(port[port.size() - 1] == 'S') {
        port.erase(port.size() - 1);
        if(CryptoManager::getInstance()->TLSOk()) {
            secure = true;
        }
    }

    if(port.empty()) {
        PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$ConnectToMe ignored: empty port")));
        return;
    }

    if(BOOLSETTING(ALLOW_NATT)) {
        if(port[port.size() - 1] == 'N') {
            if(senderNick.empty())
                return;

            port.erase(port.size() - 1);
            if(port.empty())
                return;

            ConnectionManager::getInstance()->nmdcConnect(server, port, sock->getLocalPort(),
                                                          BufferedSocket::NAT_CLIENT, getMyNick(), getHubUrl(), getEncoding(), secure);

            ConnectionManager::getInstance()->nmdcExpect(senderNick, getMyNick(), getHubUrl());
            send("$ConnectToMe " + fromUtf8(senderNick) + " " + getLocalIp() + ":" + sock->getLocalPort() + (secure ? "RS" : "R") + "|");
            return;
        } else if(port[port.size() - 1] == 'R') {
            port.erase(port.size() - 1);
            if(port.empty())
                return;

            ConnectionManager::getInstance()->nmdcConnect(server, port, sock->getLocalPort(),
                                                          BufferedSocket::NAT_SERVER, getMyNick(), getHubUrl(), getEncoding(), secure);
            return;
        }
    }

    PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$ConnectToMe from %1%:%2%") % server % port));
    ConnectionManager::getInstance()->nmdcConnect(server, port, getMyNick(), getHubUrl(), getEncoding(), secure);
}

void NmdcHub::onRevConnectToMe(const string& param) {
    if(state != STATE_NORMAL) {
        return;
    }

    string::size_type j = param.find(' ');
    if(j == string::npos) {
        return;
    }

    OnlineUser* u = findUser(param.substr(0, j));
    if(u == NULL) {
        PeerConnectLog::nmdcRecv(getHubUrl(), str(F_("$RevConnectToMe: sender %1% not in user list") % param.substr(0, j)));
        return;
    }

    if(isActive()) {
        PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, replying with $ConnectToMe");
        connectToMe(*u);
    } else if(BOOLSETTING(ALLOW_NATT) && u->getUser()->isSet(User::NAT_TRAVERSAL)) {
        PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, NAT traversal");
        bool secure = allowSecureCtm() && PeerConnectTls::resolveSecureNmdc(PeerConnectTls::AUTO, *u);
        ConnectionManager::getInstance()->nmdcExpect(fromUtf8(u->getIdentity().getNick()), getMyNick(), getHubUrl());
        send("$ConnectToMe " + fromUtf8(u->getIdentity().getNick()) + " " +
             getLocalIp() + ":" + sock->getLocalPort() +
             (secure ? "NS " : "N ") + fromUtf8(getMyNick()) + "|");
    } else if(!u->getUser()->isSet(User::PASSIVE)) {
        PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, marking remote peer passive and forwarding RevConnectToMe");
        u->getUser()->setFlag(User::PASSIVE);
        revConnectToMe(*u);
        updated(*u);
    } else {
        PeerConnectLog::nmdcRecv(*u, "$RevConnectToMe, both passive — no connection possible");
    }
}

} // namespace dcpp
