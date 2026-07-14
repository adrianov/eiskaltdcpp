/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManagerPeerMatch.h"

#include "ClientManager.h"
#include "User.h"

namespace dcpp {
namespace ConnectionManagerPeerMatch {

namespace {

bool sameNick(ClientManager* cm, const HintedUser& a, const HintedUser& b) {
    const StringList first = cm->getNicks(a);
    const StringList second = cm->getNicks(b);
    return !first.empty() && !second.empty() &&
            Util::stricmp(first[0].c_str(), second[0].c_str()) == 0;
}

bool sameIp(ClientManager* cm, const HintedUser& a, const HintedUser& b) {
    const string first = Util::normalizeIpv4(cm->getField(a.user->getCID(), a.hint, "I4"));
    const string second = Util::normalizeIpv4(cm->getField(b.user->getCID(), b.hint, "I4"));
    return !first.empty() && first == second;
}

} // namespace

bool samePeer(const HintedUser& a, const HintedUser& b) {
    if(a.user == b.user || a.user->getCID() == b.user->getCID())
        return true;
    if(!a.user->isSet(User::NMDC) || !b.user->isSet(User::NMDC))
        return false;
    if(!a.hint.empty() && a.hint == b.hint)
        return false;

    auto* cm = ClientManager::getInstance();
    const int64_t aShare = Util::toInt64(cm->getField(a.user->getCID(), a.hint, "SS"));
    const int64_t bShare = Util::toInt64(cm->getField(b.user->getCID(), b.hint, "SS"));
    if(aShare <= 0 || aShare != bShare)
        return false;

    return sameNick(cm, a, b) || sameIp(cm, a, b);
}

void forEachListPeer(const HintedUser& seed, const std::function<void(const HintedUser&)>& fn) {
    if(!seed.user)
        return;
    fn(seed);
    ClientManager::getInstance()->visitOnlineUsers([&](OnlineUser* ou) {
        HintedUser peer(ou->getUser(), ou->getClient().getHubUrl());
        if(peer.user == seed.user && peer.hint == seed.hint)
            return;
        if(samePeer(seed, peer))
            fn(peer);
    });
}

/** Seed keys only: aliases with the same nick/IP+share hash to the same keys. */
void collectListPeerKeys(const HintedUser& user, std::unordered_set<string>& keys) {
    if(!user.user)
        return;
    auto* cm = ClientManager::getInstance();
    const int64_t share = Util::toInt64(cm->getField(user.user->getCID(), user.hint, "SS"));
    if(share <= 0) {
        keys.insert(user.user->getCID().toBase32());
        return;
    }
    const string shareS = Util::toString(share);
    const string ip = Util::normalizeIpv4(cm->getField(user.user->getCID(), user.hint, "I4"));
    if(!ip.empty())
        keys.insert(ip + ":" + shareS);
    const StringList nicks = cm->getNicks(user);
    if(!nicks.empty())
        keys.insert(Text::toLower(nicks[0]) + ":" + shareS);
    if(ip.empty() && nicks.empty())
        keys.insert(user.user->getCID().toBase32());
}

} // namespace ConnectionManagerPeerMatch
} // namespace dcpp
