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

#include <unordered_set>

namespace dcpp {

using ConnectionManagerPeerMatch::collectListPeerKeys;

namespace {

HintedUser resolveUser(const UserPtr& user) {
    return HintedUser(user, ClientManager::getInstance()->resolveHubHint(user, Util::emptyString));
}

} // namespace

bool ConnectionManager::connectCooldownActive(const HintedUser& user) const {
    if(!user.user)
        return false;
    std::unordered_set<string> keys;
    collectListPeerKeys(user, keys);
    Lock l(cooldownCs);
    for(const auto& key : keys) {
        auto i = connectCooldown.find(key);
        if(i != connectCooldown.end() && GET_TICK() < i->second.until)
            return true;
    }
    return false;
}

bool ConnectionManager::connectCooldownActive(const UserPtr& user) const {
    if(!user)
        return false;
    return connectCooldownActive(resolveUser(user));
}

void ConnectionManager::noteConnectCooldown(const HintedUser& user, int minBackoffMs) {
    if(!user.user)
        return;
    std::unordered_set<string> keys;
    collectListPeerKeys(user, keys);
    Lock l(cooldownCs);
    for(const auto& key : keys) {
        auto& e = connectCooldown[key];
        e.strikes = min(e.strikes + 1, PeerConnectFilter::MAX_CONNECT_ERRORS);
        const int wait = max(minBackoffMs, PeerConnectFilter::connectBackoffMs(e.strikes));
        const uint64_t until = GET_TICK() + static_cast<uint64_t>(wait);
        if(until > e.until)
            e.until = until;
    }
}

void ConnectionManager::noteConnectCooldown(const UserPtr& user, int minBackoffMs) {
    if(!user)
        return;
    noteConnectCooldown(resolveUser(user), minBackoffMs);
}

void ConnectionManager::clearConnectCooldown(const UserPtr& user) {
    if(!user)
        return;
    std::unordered_set<string> keys;
    collectListPeerKeys(resolveUser(user), keys);
    Lock l(cooldownCs);
    for(const auto& key : keys)
        connectCooldown.erase(key);
}

bool ConnectionManager::allowOutgoingConnect(const UserPtr& user) const {
    return !connectCooldownActive(user);
}

void ConnectionManager::noteOutgoingConnect(const UserPtr& user, int minBackoffMs) {
    noteConnectCooldown(user, minBackoffMs);
}

void ConnectionManager::clearOutgoingStrikes(const UserPtr& user) {
    if(!user)
        return;
    std::unordered_set<string> keys;
    collectListPeerKeys(resolveUser(user), keys);
    Lock l(cooldownCs);
    for(const auto& key : keys) {
        auto i = connectCooldown.find(key);
        if(i != connectCooldown.end())
            i->second.strikes = 0;
    }
}

} // namespace dcpp
