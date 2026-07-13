/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ConnectionManager.h"
#include "SearchResult.h"
#include "SettingsManager.h"
#include "User.h"

namespace dcpp {

void QueueManager::noDeleteFileList(const string& path) {
    if(!BOOLSETTING(KEEP_LISTS)) {
        protectedFileLists.push_back(path);
    }
}

// SearchManagerListener
void QueueManager::on(SearchManagerListener::SR, const SearchResultPtr& sr) noexcept {
    bool added = false;
    bool wantConnection = false;

    {
        Lock l(cs);
        auto matches = fileQueue.find(sr->getTTH());

        for(auto qi : matches) {
            // Size compare to avoid popular spoof
            if(qi->getSize() != sr->getSize())
                continue;

            if(!qi->isSource(sr->getUser())) {
                try {
                    if(!BOOLSETTING(AUTO_SEARCH_AUTO_MATCH))
                        wantConnection = addSource(qi, HintedUser(sr->getUser(), sr->getHubURL()), 0);
                    added = true;
                } catch(const Exception&) {
                    // ...
                }
            } else {
                // Already on the queue item: still nudge connect (revives given-up CQIs).
                wantConnection = (qi->getPriority() != QueueItem::PAUSED)
                        && !userQueue.getRunning(sr->getUser());
            }
            break;
        }
    }

    if(added && BOOLSETTING(AUTO_SEARCH_AUTO_MATCH)
            && !sr->getUser()->isSet(User::VIRUS_INFECTED)) {
        try {
            addList(HintedUser(sr->getUser(), sr->getHubURL()), QueueItem::FLAG_MATCH_QUEUE);
        } catch(const Exception&) {
            // ...
        }
    }
    if(sr->getUser()->isOnline() && wantConnection) {
        ConnectionManager::getInstance()->getDownloadConnection(HintedUser(sr->getUser(), sr->getHubURL()));
    }

}

// ClientManagerListener
void QueueManager::on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
    bool hasDown = false;
    {
        Lock l(cs);
        for(int i = 0; i < QueueItem::LAST; ++i) {
            auto j = userQueue.getList(i).find(aUser);
            if(j != userQueue.getList(i).end()) {
                for(auto& m: j->second)
                    fire(QueueManagerListener::StatusUpdated(), m);
                if(i != QueueItem::PAUSED)
                    hasDown = true;
            }
        }
    }

    if(hasDown) {
        // the user just came on, so there's only 1 possible hub, no need for a hint
        ConnectionManager::getInstance()->getDownloadConnection(HintedUser(aUser, Util::emptyString));
    }
}

void QueueManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
    Lock l(cs);
    for(int i = 0; i < QueueItem::LAST; ++i) {
        auto j = userQueue.getList(i).find(aUser);
        if(j != userQueue.getList(i).end()) {
            for(auto& m: j->second)
                fire(QueueManagerListener::StatusUpdated(), m);
        }
    }
}

void QueueManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    if(dirty && ((lastSave + 10000) < aTick)) {
        saveQueue();
    }
}

} // namespace dcpp
