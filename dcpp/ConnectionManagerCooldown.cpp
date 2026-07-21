/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "ClientManager.h"
#include "ConnectionManagerPeerMatch.h"
#include "PeerConnectFilter.h"
#include "PeerConnectHub.h"

#include <unordered_set>

namespace dcpp {

using ConnectionManagerPeerMatch::collectListPeerKeys;

namespace {

HintedUser resolveUser(const UserPtr& user) {
    return HintedUser(user, ClientManager::getInstance()->resolveHubHint(user, Util::emptyString));
}

bool latchActive(const unordered_map<string, uint64_t>& latch, const HintedUser& user) {
    if(!user.user)
        return false;
    std::unordered_set<string> keys;
    collectListPeerKeys(user, keys);
    const uint64_t now = GET_TICK();
    for(const auto& key : keys) {
        auto i = latch.find(key);
        if(i != latch.end() && now < i->second)
            return true;
    }
    return false;
}

void latchArm(unordered_map<string, uint64_t>& latch, const HintedUser& user, int ms) {
    if(!user.user || ms <= 0)
        return;
    std::unordered_set<string> keys;
    collectListPeerKeys(user, keys);
    const uint64_t until = GET_TICK() + static_cast<uint64_t>(ms);
    for(const auto& key : keys) {
        auto& slot = latch[key];
        if(until > slot)
            slot = until;
    }
}

void latchClear(unordered_map<string, uint64_t>& latch, const HintedUser& user) {
    if(!user.user)
        return;
    std::unordered_set<string> keys;
    collectListPeerKeys(user, keys);
    for(const auto& key : keys)
        latch.erase(key);
}

} // namespace

bool ConnectionManager::allowOutgoingConnect(const UserPtr& user) const {
    if(!user)
        return false;
    // Lock order: cs then cooldownCs (same as fail / force / timer paths).
    const HintedUser hinted = resolveUser(user);
    Lock l(cs);
    if(peerConnectInFlight(hinted))
        return false;
    Lock l2(cooldownCs);
    return !latchActive(ctmLatch, hinted);
}

void ConnectionManager::noteOutgoingConnect(const UserPtr& user) {
    if(!user)
        return;
    Lock l(cooldownCs);
    latchArm(ctmLatch, resolveUser(user), PeerConnectFilter::CTM_LATCH_MS);
}

void ConnectionManager::clearOutgoingConnect(const UserPtr& user) {
    if(!user)
        return;
    Lock l(cooldownCs);
    latchClear(ctmLatch, resolveUser(user));
}

void ConnectionManager::clearOutgoingStrikes(const UserPtr& user) {
    // Peer granted a download slot: allow an immediate follow-up CTM if needed.
    if(!user)
        return;
    const HintedUser hinted = resolveUser(user);
    {
        Lock l(cs);
        if(auto* cqi = findDownloadCqi(hinted))
            cqi->setGrantedSlot(true);
    }
    clearOutgoingConnect(user);
}

void ConnectionManager::forgetDownloadSlot(const UserPtr& user) {
    if(!user)
        return;
    PeerConnectHub::notePeerReached(user);
    const HintedUser hinted = resolveUser(user);
    Lock l(cs);
    if(auto* cqi = findDownloadCqi(hinted))
        cqi->setGrantedSlot(false);
}

} // namespace dcpp
