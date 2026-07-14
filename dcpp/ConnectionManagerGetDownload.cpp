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
#include "ConnectionManagerPeerMatch.h"
#include "DownloadManager.h"
#include "PeerConnectFilter.h"
#include "QueueManager.h"
#include "User.h"

namespace dcpp {

using ConnectionManagerPeerMatch::samePeer;

void ConnectionManager::nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl) {
    expectedConnections.clearOtherHubs(aNick, aHubUrl);
    expectedConnections.add(aNick, aMyNick, aHubUrl);
}

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

bool ConnectionManager::peerConnectInFlight(const HintedUser& user) const {
    for(auto& cqi : downloads) {
        if(cqi->getState() != ConnectionQueueItem::CONNECTING)
            continue;
        if(samePeer(user, cqi->getUser()))
            return true;
    }
    return false;
}

ConnectionQueueItem* ConnectionManager::findDownloadCqiForHub(const string& hubUrl, const CID& wireCid) const {
    ConnectionQueueItem* cidMatch = nullptr;
    for(auto& cqi : downloads) {
        if(cqi->getState() != ConnectionQueueItem::CONNECTING &&
                cqi->getState() != ConnectionQueueItem::WAITING)
            continue;
        if(!(cqi->getUser().user->getCID() == wireCid))
            continue;
        if(!hubUrl.empty() && cqi->getUser().hint == hubUrl)
            return cqi;
        cidMatch = cqi;
    }
    return cidMatch;
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
    HintedUser user = aUser;
    user.hint = ClientManager::getInstance()->resolveHubHint(user.user, user.hint);
    if(user.user->isSet(User::NMDC) && Util::toInt64(ClientManager::getInstance()->getField(
            user.user->getCID(), user.hint, "SS")) <= 0)
        return;

    if(!QueueManager::getInstance()->allowDownloadConnect(user))
        return;

    UserPtr idleUser;
    {
        Lock l(cs);
        ConnectionQueueItem* cqi = findDownloadCqi(user);
        if(cqi) {
            ConnectionQueueItem::List stale;
            for(auto& item : downloads) {
                if(item != cqi && item->getState() != ConnectionQueueItem::ACTIVE &&
                        item->getState() != ConnectionQueueItem::CONNECTING &&
                        samePeer(user, item->getUser()))
                    stale.push_back(item);
            }
            for(auto& item : stale) {
                mergeQueueState(cqi, item);
                putCQI(item);
            }
            if(cqi->getState() == ConnectionQueueItem::WAITING)
                cqi->setUser(user);
            else if(!user.hint.empty() && cqi->getUser().hint.empty())
                cqi->setHubHint(user.hint);
            if(cqi->getState() != ConnectionQueueItem::ACTIVE &&
                    cqi->getState() != ConnectionQueueItem::CONNECTING) {
                reviveDownloadQueue(cqi);
                if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS)
                    cqi->setState(ConnectionQueueItem::WAITING);
            }
            idleUser = cqi->getUser().user;
        } else {
            getCQI(user, true);
        }
    }
    if(idleUser)
        DownloadManager::getInstance()->checkIdle(idleUser);
}

} // namespace dcpp
