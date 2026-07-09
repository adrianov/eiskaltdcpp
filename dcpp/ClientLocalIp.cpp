/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "Client.h"

#include "ClientManager.h"
#include "SettingsManager.h"
#include "Socket.h"
#include "Util.h"

namespace dcpp {

namespace {

string hubReportedIp() {
    auto cm = ClientManager::getInstance();
    auto lock = cm->lock();
    for(auto c: cm->getClients()) {
        const string ip = Util::normalizeIpv4(c->getMyIdentity().getIp());
        if(!ip.empty() && !Util::isPrivateIp(ip))
            return ip;
    }
    return Util::emptyString;
}

} // namespace

string Client::getLocalIp() const {
    string ip;
    if(!externalIP.empty())
        ip = Util::normalizeIpv4(Socket::resolve(externalIP));
    if(ip.empty() && (!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()))
        ip = Util::normalizeIpv4(getMyIdentity().getIp());
    if(ip.empty() && !SETTING(EXTERNAL_IP).empty())
        ip = Util::normalizeIpv4(Socket::resolve(SETTING(EXTERNAL_IP)));
    if(ip.empty() && !Util::isPrivateIp(getIp()))
        ip = hubReportedIp();
    if(ip.empty())
        ip = Util::normalizeIpv4(localIp);
    if(ip.empty())
        ip = Util::normalizeIpv4(Util::getLocalIp(AF_INET));
    return ip;
}

} // namespace dcpp
