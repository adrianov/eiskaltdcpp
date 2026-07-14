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
#include "ListCache.h"
#include "User.h"

namespace dcpp {
namespace ConnectionManagerPeerMatch {

namespace {

bool knownDifferent(ClientManager* cm, const HintedUser& a, const HintedUser& b, const char* field) {
    const string first = cm->getField(a.user->getCID(), a.hint, field);
    const string second = cm->getField(b.user->getCID(), b.hint, field);
    return !first.empty() && !second.empty() && first != second;
}

bool sameNick(ClientManager* cm, const HintedUser& a, const HintedUser& b) {
    const StringList first = cm->getNicks(a);
    const StringList second = cm->getNicks(b);
    return !first.empty() && !second.empty() &&
            Util::stricmp(first[0].c_str(), second[0].c_str()) == 0;
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
    if(aShare > 0 && aShare == bShare && sameNick(cm, a, b))
        return true;
    if(aShare <= 0 || bShare <= 0 || aShare != bShare)
        return false;
    if(knownDifferent(cm, a, b, "TA") || knownDifferent(cm, a, b, "DE"))
        return false;

    const string aIp = Util::normalizeIpv4(cm->getField(a.user->getCID(), a.hint, "I4"));
    const string bIp = Util::normalizeIpv4(cm->getField(b.user->getCID(), b.hint, "I4"));
    if(!aIp.empty() && !bIp.empty() && aIp != bIp)
        return false;

    const int64_t aList = ListCache::fileSize(a.user->getCID());
    const int64_t bList = ListCache::fileSize(b.user->getCID());
    return aList < 0 || bList < 0 || aList == bList;
}

} // namespace ConnectionManagerPeerMatch
} // namespace dcpp
