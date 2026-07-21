/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectHub.h"

#include "User.h"

#include <unordered_set>

namespace dcpp {

namespace PeerConnectHub {

namespace {

FastCriticalSection skipCs;
unordered_map<CID, unordered_set<string>> timeoutHubs;
unordered_set<CID> reachedPeers;
enum { MAX_SKIP_USERS = 2048 };

void trimSkipMap(const CID& cid) {
    if(timeoutHubs.size() >= MAX_SKIP_USERS && timeoutHubs.find(cid) == timeoutHubs.end())
        timeoutHubs.erase(timeoutHubs.begin());
}

} // namespace

void noteConnectTimeout(const UserPtr& user, const string& hub) {
    if(!user || hub.empty())
        return;
    FastLock l(skipCs);
    const CID& cid = user->getCID();
    trimSkipMap(cid);
    timeoutHubs[cid].insert(hub);
}

bool isConnectTimeoutHub(const UserPtr& user, const string& hub) {
    if(!user || hub.empty())
        return false;
    FastLock l(skipCs);
    auto i = timeoutHubs.find(user->getCID());
    if(i == timeoutHubs.end())
        return false;
    return i->second.find(hub) != i->second.end();
}

void clearConnectTimeouts(const UserPtr& user) {
    if(!user)
        return;
    FastLock l(skipCs);
    timeoutHubs.erase(user->getCID());
}

void notePeerReached(const UserPtr& user) {
    if(!user)
        return;
    FastLock l(skipCs);
    if(reachedPeers.size() >= MAX_SKIP_USERS && reachedPeers.find(user->getCID()) == reachedPeers.end())
        reachedPeers.erase(reachedPeers.begin());
    reachedPeers.insert(user->getCID());
}

bool wasPeerReached(const UserPtr& user) {
    if(!user)
        return false;
    FastLock l(skipCs);
    return reachedPeers.find(user->getCID()) != reachedPeers.end();
}

void clearPeerSession(const UserPtr& user) {
    if(!user)
        return;
    FastLock l(skipCs);
    const CID& cid = user->getCID();
    timeoutHubs.erase(cid);
    reachedPeers.erase(cid);
}

} // namespace PeerConnectHub

} // namespace dcpp
