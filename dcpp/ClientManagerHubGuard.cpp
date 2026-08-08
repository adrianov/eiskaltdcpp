/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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

void hubEndpoint(const string& url, string& scheme, string& host, string& port) {
    string proto, file, query, fragment;
    Util::decodeUrl(url, proto, host, port, file, query, fragment);
    if(Util::stricmp(proto.c_str(), "nmdcs") == 0)
        scheme = "nmdcs";
    else if(Util::stricmp(proto.c_str(), "adcs") == 0)
        scheme = "adcs";
    else if(Util::stricmp(proto.c_str(), "adc") == 0)
        scheme = "adc";
    else
        scheme = "nmdc"; // empty / dchub / nmdc
    if(port.empty() && !host.empty() && (scheme == "nmdc" || scheme == "nmdcs"))
        port = "411";
}

bool matchHub(const string& a, const string& b, bool withScheme) {
    if(a == b)
        return true;

    string aScheme, aHost, aPort, bScheme, bHost, bPort;
    hubEndpoint(a, aScheme, aHost, aPort);
    hubEndpoint(b, bScheme, bHost, bPort);
    if(aHost.empty() || bHost.empty())
        return false;
    if(Util::stricmp(aHost.c_str(), bHost.c_str()) != 0 || aPort != bPort)
        return false;
    return !withScheme || aScheme == bScheme;
}

} // namespace

bool sameHubUrl(const string& a, const string& b) {
    return matchHub(a, b, true);
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
        // Host+port only: plain and TLS to the same hub count as one presence.
        if(matchHub(c->getHubUrl(), url, false))
            return true;
        if(!name.empty() && Util::stricmp(c->getHubName().c_str(), name.c_str()) == 0)
            return true;
    }
    return false;
}

} // namespace ClientManagerHubGuard
} // namespace dcpp
