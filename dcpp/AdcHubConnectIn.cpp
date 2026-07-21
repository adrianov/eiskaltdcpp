/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "format.h"
#include "PeerConnectLog.h"
#include "SettingsManager.h"

namespace dcpp {

void AdcHub::handle(AdcCommand::CTM, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 3)
        return;

    OnlineUser* u = findUser(c.getFrom());
    if(!u || u->getUser() == ClientManager::getInstance()->getMe())
        return;

    const string& protocol = c.getParam(0);
    const string& port = c.getParam(1);
    const string& token = c.getParam(2);

    bool secure = false;
    if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
        // Nothing special
    } else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk()) {
        secure = true;
    } else {
        unknownProtocol(c.getFrom(), protocol, token);
        return;
    }

    if(!u->getIdentity().isTcpActive()) {
        send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "IP unknown", AdcCommand::TYPE_DIRECT).setTo(c.getFrom()));
        return;
    }

    ConnectionManager::getInstance()->adcConnect(*u, port, token, secure);
}

void AdcHub::handle(AdcCommand::RCM, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 2)
        return;

    OnlineUser* u = findUser(c.getFrom());
    if(!u || u->getUser() == ClientManager::getInstance()->getMe())
        return;

    const string& protocol = c.getParam(0);
    const string& token = c.getParam(1);

    bool secure;
    if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
        secure = false;
    } else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk()) {
        secure = true;
    } else {
        unknownProtocol(c.getFrom(), protocol, token);
        return;
    }

    auto* cm = ConnectionManager::getInstance();
    if(!cm->allowOutgoingConnect(u->getUser())) {
        PeerConnectLog::skip(u->getIdentity().getNick(), getHubUrl(),
                _("recent CTM/RCM still in flight"));
        return;
    }

    if(isActive()) {
        connectSecure(*u, token, secure);
        return;
    }

    if(!BOOLSETTING(ALLOW_NATT) || !u->getIdentity().supports(NAT0_FEATURE))
        return;

    cm->noteOutgoingConnect(u->getUser());
    send(AdcCommand(AdcCommand::CMD_NAT, u->getIdentity().getSID(), AdcCommand::TYPE_DIRECT).
         addParam(protocol).addParam(sock->getLocalPort()).addParam(token));
}

void AdcHub::handle(AdcCommand::NAT, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 3)
        return;

    if(!BOOLSETTING(ALLOW_NATT))
        return;

    OnlineUser* u = findUser(c.getFrom());
    if(!u || u->getUser() == ClientManager::getInstance()->getMe())
        return;

    const string& protocol = c.getParam(0);
    const string& port = c.getParam(1);
    const string& token = c.getParam(2);

    bool secure = false;
    if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
        // Nothing special
    } else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk()) {
        secure = true;
    } else {
        unknownProtocol(c.getFrom(), protocol, token);
        return;
    }

    dcdebug("triggering connecting attempt in NAT: remote port = %s, local IP = %s, local port = %s\n", port.c_str(), sock->getLocalIp().c_str(), sock->getLocalPort().c_str());
    ConnectionManager::getInstance()->adcConnect(*u, port, sock->getLocalPort(), BufferedSocket::NAT_CLIENT, token, secure);

    send(AdcCommand(AdcCommand::CMD_RNT, u->getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(protocol).
         addParam(sock->getLocalPort()).addParam(token));
}

void AdcHub::handle(AdcCommand::RNT, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 3)
        return;

    if(!BOOLSETTING(ALLOW_NATT))
        return;

    OnlineUser* u = findUser(c.getFrom());
    if(!u || u->getUser() == ClientManager::getInstance()->getMe())
        return;

    const string& protocol = c.getParam(0);
    const string& port = c.getParam(1);
    const string& token = c.getParam(2);

    bool secure = false;
    if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
        // Nothing special
    } else if(protocol == SECURE_CLIENT_PROTOCOL_TEST && CryptoManager::getInstance()->TLSOk()) {
        secure = true;
    } else {
        unknownProtocol(c.getFrom(), protocol, token);
        return;
    }

    dcdebug("triggering connecting attempt in RNT: remote port = %s, local IP = %s, local port = %s\n", port.c_str(), sock->getLocalIp().c_str(), sock->getLocalPort().c_str());
    ConnectionManager::getInstance()->adcConnect(*u, port, sock->getLocalPort(), BufferedSocket::NAT_SERVER, token, secure);
}

} // namespace dcpp
