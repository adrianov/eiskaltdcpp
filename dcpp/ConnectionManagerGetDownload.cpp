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

        const CID& cid = aUser.user->getCID();
        StringList nicks = ClientManager::getInstance()->getNicks(aUser);
        if(!nicks.empty()) {
            std::unordered_set<CID> sameNick;
            ClientManager::getInstance()->cidsForNick(nicks[0], sameNick);
            ConnectionQueueItem::List stale;
            for(auto& cqi : downloads) {
                if(cqi->getUser().user == aUser.user)
                    continue;
                if(cqi->getUser().user->getCID() == cid)
                    continue;
                if(sameNick.count(cqi->getUser().user->getCID()))
                    stale.push_back(cqi);
            }
            for(auto& cqi : stale)
                putCQI(cqi);
        }

        auto* cqi = findDownloadCqi(aUser.user);
        if(cqi) {
            cqi->setHubHint(aUser.hint);
            DownloadManager::getInstance()->checkIdle(aUser.user);
        } else {
            getCQI(aUser, true);
        }
    }
}

} // namespace dcpp
