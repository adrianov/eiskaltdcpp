/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ClientManagerHubGuard.h"

#include "Client.h"
#include "ClientManager.h"
#include "Util.h"

namespace dcpp {
namespace ClientManagerHubGuard {

namespace {

void hubEndpoint(const string& url, string& host, string& port) {
    string proto, file, query, fragment;
    Util::decodeUrl(url, proto, host, port, file, query, fragment);
    if(!port.empty() || host.empty())
        return;
    if(Util::stricmp(proto.c_str(), "nmdcs") == 0 || Util::stricmp(proto.c_str(), "dchub") == 0 || proto.empty())
        port = "411";
}

} // namespace

bool sameHubUrl(const string& a, const string& b) {
    if(a == b)
        return true;

    string aHost, aPort, bHost, bPort;
    hubEndpoint(a, aHost, aPort);
    hubEndpoint(b, bHost, bPort);
    if(aHost.empty() || bHost.empty())
        return false;

    return Util::stricmp(aHost.c_str(), bHost.c_str()) == 0 && aPort == bPort;
}

bool hasActiveHub(const string& url, const Client* exclude) {
    return hasActiveHub(url, Util::emptyString, exclude);
}

bool hasActiveHub(const string& url, const string& name, const Client* exclude) {
    auto cm = ClientManager::getInstance();
    auto lock = cm->lock();
    for(auto c: cm->getClients()) {
        if(c == exclude || !c->isConnected())
            continue;
        if(sameHubUrl(c->getHubUrl(), url))
            return true;
        if(!name.empty() && Util::stricmp(c->getHubName().c_str(), name.c_str()) == 0)
            return true;
    }
    return false;
}

} // namespace ClientManagerHubGuard
} // namespace dcpp
