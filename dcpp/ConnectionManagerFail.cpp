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

#include "ClientManager.h"
#include "Encoder.h"
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

void ConnectionManager::rejectConnection(UserConnection* aConn, const string& reason) {
    PeerConnectLog::connectionReject(aConn, reason);
    putConnection(aConn);
}

bool ConnectionManager::checkKeyprint(UserConnection *aSource) {
    auto kp = aSource->getKeyprint();
    if(kp.empty()) {
        return true;
    }

    auto kp2 = ClientManager::getInstance()->getField(aSource->getUser()->getCID(), aSource->getHubUrl(), "KP");
    if(kp2.empty()) {
        return true;
    }

    if(kp2.compare(0, 7, "SHA256/") != 0) {
        return true;
    }

    dcdebug("Keyprint: %s vs %s\n", Encoder::toBase32(&kp[0], kp.size()).c_str(), kp2.c_str() + 7);

    vector<uint8_t> kp2v(kp.size());
    Encoder::fromBase32(&kp2[7], &kp2v[0], kp2v.size());
    if(!std::equal(kp.begin(), kp.end(), kp2v.begin())) {
        dcdebug("Not equal...\n");
        return false;
    }

    return true;
}

bool ConnectionManager::checkHubCCBlock(const string& aServer, const string& aPort, const string& aHubUrl)
{
    const auto server_lower = Text::toLower(aServer);
    dcassert(server_lower == aServer);

    bool cc_blocked = false;

    {
        Lock l(cs);
        cc_blocked = !hubsBlockingCC.empty() && hubsBlockingCC.find(server_lower) != hubsBlockingCC.end();
    }

    if(cc_blocked)
    {
        PeerConnectLog::skip(aServer, aHubUrl, str(F_("blocked hub IP (C-C protection) %1%:%2%") % aServer % aPort));
        LogManager::getInstance()->message(str(F_("Blocked a C-C connection to a hub ('%1%:%2%'; request from '%3%')") % aServer % aPort % aHubUrl));
        return true;
    }
    return false;
}

} // namespace dcpp
