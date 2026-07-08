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

#include <unordered_set>

namespace dcpp {

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

        auto i = find(downloads.begin(), downloads.end(), aUser.user);
        if(i == downloads.end()) {
            for(auto& cqi : downloads) {
                if(cqi->getUser().user->getCID() == cid) {
                    cqi->setHubHint(aUser.hint);
                    DownloadManager::getInstance()->checkIdle(aUser.user);
                    return;
                }
            }
            getCQI(aUser, true);
        } else {
            DownloadManager::getInstance()->checkIdle(aUser.user);
        }
    }
}

} // namespace dcpp
