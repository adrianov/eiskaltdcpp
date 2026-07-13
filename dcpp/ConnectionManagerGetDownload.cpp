/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "DownloadManager.h"
#include "ListCache.h"
#include "PeerConnectFilter.h"
#include "User.h"

namespace dcpp {

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

/** Treat cross-hub NMDC identities as aliases unless stronger metadata disagrees. */
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

} // namespace

void ConnectionManager::mergeQueueState(ConnectionQueueItem* keep, const ConnectionQueueItem* other) {
    if(!keep || !other || keep == other)
        return;
    if(other->getErrors() > keep->getErrors())
        keep->setErrors(other->getErrors());
    if(other->getLastAttempt() > keep->getLastAttempt())
        keep->setLastAttempt(other->getLastAttempt());
    if(other->getSlotWaits() > keep->getSlotWaits())
        keep->setSlotWaits(other->getSlotWaits());
    if(other->getConnectAttempts() > keep->getConnectAttempts())
        keep->setConnectAttempts(other->getConnectAttempts());
}

bool ConnectionManager::connectCooldownActive(const UserPtr& user) const {
    if(!user)
        return false;
    auto i = connectCooldown.find(user->getCID());
    return i != connectCooldown.end() && GET_TICK() < i->second.until;
}

void ConnectionManager::noteConnectCooldown(const UserPtr& user, int minBackoffMs) {
    if(!user)
        return;
    auto& e = connectCooldown[user->getCID()];
    e.strikes = min(e.strikes + 1, PeerConnectFilter::MAX_CONNECT_ERRORS);
    const int wait = max(minBackoffMs, PeerConnectFilter::connectBackoffMs(e.strikes));
    const uint64_t until = GET_TICK() + static_cast<uint64_t>(wait);
    if(until > e.until)
        e.until = until;
}

void ConnectionManager::clearConnectCooldown(const UserPtr& user) {
    if(user)
        connectCooldown.erase(user->getCID());
}

bool ConnectionManager::allowOutgoingConnect(const UserPtr& user) const {
    Lock l(cs);
    return !connectCooldownActive(user);
}

void ConnectionManager::noteOutgoingConnect(const UserPtr& user, int minBackoffMs) {
    Lock l(cs);
    noteConnectCooldown(user, minBackoffMs);
}

void ConnectionManager::clearOutgoingStrikes(const UserPtr& user) {
    if(!user)
        return;
    Lock l(cs);
    auto i = connectCooldown.find(user->getCID());
    if(i != connectCooldown.end())
        i->second.strikes = 0;
}

ConnectionQueueItem* ConnectionManager::findDownloadCqi(const HintedUser& user) {
    ConnectionQueueItem* match = nullptr;
    for(auto& cqi : downloads) {
        if(!samePeer(user, cqi->getUser()))
            continue;
        if(!match || (match->getState() != ConnectionQueueItem::ACTIVE &&
                (cqi->getState() == ConnectionQueueItem::ACTIVE ||
                 cqi->getState() == ConnectionQueueItem::CONNECTING ||
                 (match->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS &&
                  cqi->getState() == ConnectionQueueItem::WAITING))))
            match = cqi;
    }
    return match;
}

bool ConnectionManager::slotWaitActive(const ConnectionQueueItem* cqi) const {
    if(!cqi || cqi->getSlotWaits() <= 0)
        return false;
    return GET_TICK() < cqi->getLastAttempt() + static_cast<uint64_t>(
            PeerConnectFilter::queueBackoffMs(cqi->getErrors(), cqi->getSlotWaits()));
}

bool ConnectionManager::queueBackoffActive(const ConnectionQueueItem* cqi) const {
    if(!cqi)
        return false;
    if(cqi->getErrors() == -1)
        return true;
    if(connectCooldownActive(cqi->getUser().user))
        return true;
    if(cqi->getLastAttempt() == 0)
        return false;
    return GET_TICK() < cqi->getLastAttempt() + static_cast<uint64_t>(
            PeerConnectFilter::queueBackoffMs(cqi->getErrors(), cqi->getSlotWaits()));
}

void ConnectionManager::getDownloadConnection(const HintedUser& aUser) {
    dcassert((bool)aUser.user);
    if(aUser.user->isSet(User::NMDC) && Util::toInt64(ClientManager::getInstance()->getField(
            aUser.user->getCID(), aUser.hint, "SS")) <= 0)
        return;

    UserPtr idleUser;
    {
        Lock l(cs);
        ConnectionQueueItem* cqi = findDownloadCqi(aUser);
        if(cqi) {
            ConnectionQueueItem::List stale;
            for(auto& item : downloads) {
                if(item != cqi && item->getState() != ConnectionQueueItem::ACTIVE &&
                        item->getState() != ConnectionQueueItem::CONNECTING &&
                        samePeer(aUser, item->getUser()))
                    stale.push_back(item);
            }
            for(auto& item : stale) {
                mergeQueueState(cqi, item);
                putCQI(item);
            }
            if(cqi->getState() == ConnectionQueueItem::WAITING)
                cqi->setUser(aUser);
            // Explicit reconnect must undo permanent give-up (WAITING or slot-wait).
            if(cqi->getState() != ConnectionQueueItem::ACTIVE &&
                    cqi->getState() != ConnectionQueueItem::CONNECTING) {
                reviveDownloadQueue(cqi);
                if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS)
                    cqi->setState(ConnectionQueueItem::WAITING);
            }
            idleUser = cqi->getUser().user;
        } else {
            getCQI(aUser, true);
        }
    }
    // Outside ConnectionManager::cs — checkIdle takes DownloadManager::cs.
    if(idleUser)
        DownloadManager::getInstance()->checkIdle(idleUser);
}

} // namespace dcpp
