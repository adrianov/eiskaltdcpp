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
#include "Client.h"

#include "BufferedSocket.h"
#include "ClientManager.h"
#include "DebugManager.h"
#include "Encoder.h"
#include "FavoriteManager.h"
#include "format.h"
#include "SettingsManager.h"
#include "Socket.h"
#include "Text.h"
#include "version.h"

namespace dcpp {

void Client::reloadSettings(bool updateNick) {
    auto fav = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());

    string ClientId;
    if (::strncmp(getHubUrl().c_str(),"adc://", 6) == 0 ||
            ::strncmp(getHubUrl().c_str(),"adcs://", 7) == 0)
        ClientId = fullADCVersionString;
    else
        ClientId = fullNMDCVersionString;

    if(fav) {
        if(updateNick) {
            string nick = FavoriteManager::getInstance()->getHubNick(getHubUrl());
            if(nick.empty())
                nick = fav->getNick(true);
            setCurrentNick(checkNick(nick));
        }

        if(!fav->getUserDescription().empty()) {
            setCurrentDescription(fav->getUserDescription());
        } else {
            setCurrentDescription(SETTING(DESCRIPTION));
        }

        if(!fav->getPassword().empty())
            setPassword(fav->getPassword());
        if (fav->getOverrideId() && strlen(fav->getClientId().c_str()) > 1)
            ClientId = fav->getClientId();
        if (!fav->getExternalIP().empty())
            externalIP = fav->getExternalIP();
        if (!fav->getEncoding().empty()){
            setEncoding(fav->getEncoding());
        }
        if (fav->getUseInternetIP() && !SETTING(INTERNETIP).empty()){
            externalIP = SETTING(INTERNETIP);
        }

        setSearchInterval(fav->getSearchInterval());
    } else {
        if(updateNick) {
            string nick = FavoriteManager::getInstance()->getHubNick(getHubUrl());
            if(nick.empty())
                nick = SETTING(NICK);
            setCurrentNick(checkNick(nick));
        }
        setCurrentDescription(SETTING(DESCRIPTION));
        setSearchInterval(SETTING(MINIMUM_SEARCH_INTERVAL));
    }
    setClientId(ClientId);
}

void Client::connect() {
    if(sock) {
        BufferedSocket::putSocket(sock);
        sock = 0;
    }

    setAutoReconnect(true);
    setReconnDelay(SETTING(RECONNECT_DELAY));
    reloadSettings(true);
    setRegistered(false);
    setMyIdentity(Identity(ClientManager::getInstance()->getMe(), 0));
    setHubIdentity(Identity());

    state = STATE_CONNECTING;

    try {
        sock = BufferedSocket::getSocket(separator);
        sock->addListener(this);
        sock->connect(address, port, secure, BOOLSETTING(ALLOW_UNTRUSTED_HUBS), true, proto);
    } catch(const Exception& e) {
        shutdown();
        /// @todo at this point, this hub instance is completely useless
        fire(ClientListener::Failed(), this, e.getError());
    }
    updateActivity();
}

void Client::send(const char* aMessage, size_t aLen) {
    if(!isReady()) {
        dcassert(0);
        return;
    }
    updateActivity();
    sock->write(aMessage, aLen);
    COMMAND_DEBUG((Util::stricmp(getEncoding(), Text::utf8) != 0 ? Text::toUtf8(aMessage, getEncoding()) : aMessage), DebugManager::HUB_OUT, getIpPort());
}

void Client::on(Connected) noexcept {
    setSearchBlocked(false);
    updateActivity();
    ip = sock->getIp();
    localIp = sock->getLocalIp();
    if(sock->isSecure() && keyprint.compare(0, 7, "SHA256/") == 0) {
        auto kp = sock->getKeyprint();
        if(!kp.empty()) {
            vector<uint8_t> kp2v(kp.size());
            Encoder::fromBase32(keyprint.c_str() + 7, &kp2v[0], kp2v.size());
            if(!std::equal(kp.begin(), kp.end(), kp2v.begin())) {
                state = STATE_DISCONNECTED;
                sock->removeListener(this);
                fire(ClientListener::Failed(), this, "Keyprint mismatch");
                return;
            }
        }
    }
    fire(ClientListener::Connected(), this);
    state = STATE_PROTOCOL;
}

void Client::on(Failed, const string& aLine) noexcept {
    state = STATE_DISCONNECTED;
    FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
    sock->removeListener(this);
    fire(ClientListener::Failed(), this, aLine);
}

void Client::disconnect(bool graceLess) {
    if(sock)
        sock->disconnect(graceLess);
}

bool Client::isSecure() const {
    return isReady() && sock->isSecure();
}

bool Client::isTrusted() const {
    return isReady() && sock->isTrusted();
}

std::string Client::getCipherName() const {
    return isReady() ? sock->getCipherName() : Util::emptyString;
}

vector<uint8_t> Client::getKeyprint() const {
    return isReady() ? sock->getKeyprint() : vector<uint8_t>();
}

// Public IP that some connected hub reported for us ($UserIP / ADC INF).
static string hubReportedIp() {
    auto cm = ClientManager::getInstance();
    auto lock = cm->lock();
    for(auto c: cm->getClients()) {
        const string ip = Util::normalizeIpv4(c->getMyIdentity().getIp());
        if(!ip.empty() && !Util::isPrivateIp(ip))
            return ip;
    }
    return Util::emptyString;
}

// Each candidate is validated; invalid ones fall through to the next source.
string Client::getLocalIp() const {
    string ip;
    if(!externalIP.empty())
        ip = Util::normalizeIpv4(Socket::resolve(externalIP));
    if(ip.empty() && (!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()))
        ip = Util::normalizeIpv4(getMyIdentity().getIp());
    if(ip.empty() && !SETTING(EXTERNAL_IP).empty())
        ip = Util::normalizeIpv4(Socket::resolve(SETTING(EXTERNAL_IP)));
    // On a public hub a LAN address is known-wrong; prefer an IP another hub told us.
    if(ip.empty() && !Util::isPrivateIp(getIp()))
        ip = hubReportedIp();
    if(ip.empty())
        ip = Util::normalizeIpv4(localIp);
    if(ip.empty())
        ip = Util::normalizeIpv4(Util::getLocalIp(AF_INET));
    return ip;
}

} // namespace dcpp
