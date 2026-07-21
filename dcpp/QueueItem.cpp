/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "QueueItem.h"

#include "ClientManager.h"
#include "PeerConnectFilter.h"
#include "PeerConnectHub.h"
#include "QueueManager.h"

namespace dcpp {

void QueueItem::getOnlineUsers(HintedUserList& l, const unordered_set<CID>& queuedDownloads) const {
    auto* qm = QueueManager::getInstance();
    auto* cm = ClientManager::getInstance();
    HintedUserList candidates;
    for(auto& i: sources) {
        if(!i.getUser().user->isOnline())
            continue;
        OnlineUser* ou = cm->findOnlineUser(i.getUser(), false);
        if(ou && !PeerConnectFilter::isViablePeer(*ou))
            continue;
        if(!qm->shouldConnectSource(this, i.getUser(), queuedDownloads))
            continue;
        HintedUser hu = i.getUser();
        hu.hint = cm->resolveHubHint(hu.user, hu.hint);
        candidates.push_back(hu);
    }
    if(candidates.empty())
        return;
    PeerConnectHub::sortSources(candidates);
    if(isSet(FLAG_USER_LIST))
        l.push_back(candidates.front());
    else
        l.insert(l.end(), candidates.begin(), candidates.end());
}

void QueueItem::addSource(const HintedUser& aUser) {
    dcassert(!isSource(aUser.user));
    auto i = getBadSource(aUser);
    if(i != badSources.end()) {
        sources.push_back(*i);
        badSources.erase(i);
    } else {
        sources.emplace_back(aUser);
    }
}

void QueueItem::removeSource(const UserPtr& aUser, int reason) {
    auto i = getSource(aUser);
    dcassert(i != sources.end());
    // Unreachable: drop from the item only — do not keep a bad-source exclusion.
    if(reason == Source::FLAG_UNREACHABLE) {
        sources.erase(i);
        return;
    }
    i->setFlag(reason);
    badSources.push_back(*i);
    sources.erase(i);
}

}
