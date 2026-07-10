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
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "SettingsManager.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

void ConnectionManager::nmdcConnect(const string& aServer, const string& aPort, const string& aNick, const string& hubUrl, const string& encoding, bool secure) {
    nmdcConnect(aServer, aPort, Util::emptyString, BufferedSocket::NAT_NONE, aNick, hubUrl, encoding, secure);
}

void ConnectionManager::nmdcConnect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding, bool secure) {
    if(shuttingDown)
        return;

    if(checkHubCCBlock(aServer, aPort, hubUrl))
        return;

    // Skip $ConnectToMe only when an idle upload already holds this IP (spam).
    // Running transfers may open more sockets when ALLOW_SIM_UPLOADS is enabled.
    {
        Lock l(cs);
        for(auto* existing : userConnections) {
            if(!existing->isSet(UserConnection::FLAG_UPLOAD) ||
                    !existing->isSet(UserConnection::FLAG_ASSOCIATED))
                continue;
            if(existing->getRemoteIp() != aServer)
                continue;
            if(existing->getState() == UserConnection::STATE_GET ||
                    !BOOLSETTING(ALLOW_SIM_UPLOADS))
                return;
        }
    }

    UserConnection* uc = getConnection(true, secure);
    uc->setToken(aNick);
    uc->setHubUrl(hubUrl);
    uc->setEncoding(encoding);
    uc->setState(UserConnection::STATE_CONNECT);
    uc->setFlag(UserConnection::FLAG_NMDC);
    PeerConnectLog::tcpOut(aNick, aServer, aPort, secure, "NMDC");
    try {
        uc->connect(aServer, aPort, localPort, natRole);
    } catch(const Exception& e) {
        PeerConnectLog::tcpFail(aNick, aServer, aPort, e.getError());
        LogManager::getInstance()->message(str(F_("NMDC connect to %1%:%2% failed: %3%") % aServer % aPort % e.getError()));
        putConnection(uc);
        delete uc;
    }
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string &aPort, const string& aToken, bool secure) {
    adcConnect(aUser, aPort, Util::emptyString, BufferedSocket::NAT_NONE, aToken, secure);
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string &aPort, const string &localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure) {
    if(shuttingDown)
        return;

    UserConnection* uc = getConnection(false, secure);
    uc->setToken(aToken);
    uc->setUser(aUser.getUser());
    uc->setEncoding(Text::utf8);
    uc->setState(UserConnection::STATE_CONNECT);
#ifdef WITH_DHT
    uc->setHubUrl(aUser.getClient().getType() == Client::DHT ? "DHT" : aUser.getClient().getHubUrl());
#else
    uc->setHubUrl(aUser.getClient().getHubUrl());
#endif
    if(aUser.getIdentity().isOp()) {
        uc->setFlag(UserConnection::FLAG_OP);
    }
    PeerConnectLog::tcpOut(aUser.getIdentity().getNick(), aUser.getIdentity().getIp(), aPort, secure, "ADC");
    try {
        uc->connect(aUser.getIdentity().getIp(), aPort, localPort, natRole);
    } catch(const Exception& e) {
        PeerConnectLog::tcpFail(aUser.getIdentity().getNick(), aUser.getIdentity().getIp(), aPort, e.getError());
        putConnection(uc);
        delete uc;
    }
}

} // namespace dcpp
