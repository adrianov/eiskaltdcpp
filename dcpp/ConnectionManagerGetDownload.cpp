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
#include "PeerConnectFilter.h"

#include <unordered_set>

namespace dcpp {

namespace {

void mergeQueueState(ConnectionQueueItem* keep, const ConnectionQueueItem* other) {
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

} // namespace

ConnectionQueueItem* ConnectionManager::findDownloadCqi(const UserPtr& user) {
    auto i = find(downloads.begin(), downloads.end(), user);
    if(i != downloads.end())
        return *i;

    const CID& cid = user->getCID();
    for(auto& cqi : downloads) {
        if(cqi->getUser().user->getCID() == cid)
            return cqi;
    }

    StringList nicks = ClientManager::getInstance()->getNicks(HintedUser(user, Util::emptyString));
    if(nicks.empty())
        return nullptr;

    std::unordered_set<CID> sameNick;
    ClientManager::getInstance()->cidsForNick(nicks[0], sameNick);
    for(auto& cqi : downloads) {
        if(sameNick.count(cqi->getUser().user->getCID()))
            return cqi;
    }
    return nullptr;
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
    if(cqi->getLastAttempt() == 0)
        return false;
    return GET_TICK() < cqi->getLastAttempt() + static_cast<uint64_t>(
            PeerConnectFilter::queueBackoffMs(cqi->getErrors(), cqi->getSlotWaits()));
}

void ConnectionManager::getDownloadConnection(const HintedUser& aUser) {
    dcassert((bool)aUser.user);
    {
        Lock l(cs);

        StringList nicks = ClientManager::getInstance()->getNicks(aUser);
        std::unordered_set<CID> sameNick;
        if(!nicks.empty())
            ClientManager::getInstance()->cidsForNick(nicks[0], sameNick);

        ConnectionQueueItem* cqi = findDownloadCqi(aUser.user);
        if(!cqi && !sameNick.empty()) {
            for(auto& item : downloads) {
                if(sameNick.count(item->getUser().user->getCID())) {
                    cqi = item;
                    break;
                }
            }
        }

        if(!sameNick.empty()) {
            ConnectionQueueItem::List stale;
            for(auto& item : downloads) {
                if(item == cqi)
                    continue;
                if(item->getUser().user == aUser.user)
                    continue;
                if(item->getUser().user->getCID() == aUser.user->getCID())
                    continue;
                if(sameNick.count(item->getUser().user->getCID()))
                    stale.push_back(item);
            }
            for(auto& item : stale) {
                mergeQueueState(cqi, item);
                putCQI(item);
            }
        }

        if(cqi) {
            cqi->setUser(aUser);
            DownloadManager::getInstance()->checkIdle(aUser.user);
        } else {
            getCQI(aUser, true);
        }
    }
}

} // namespace dcpp
