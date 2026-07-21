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
#include "ConnectionManagerPeerMatch.h"
#include "Download.h"
#include "File.h"

#include <unordered_set>

namespace dcpp {

void QueueManager::removePeerSources(const HintedUser& peer, int reason) noexcept {
    if(!peer.user)
        return;

    UserList users;
    {
        Lock l(cs);
        unordered_set<CID> seen;
        auto note = [&](const UserPtr& u) {
            if(u && seen.insert(u->getCID()).second)
                users.push_back(u);
        };
        note(peer.user);
        for(const auto& item: fileQueue.getQueue()) {
            for(const auto& s: item.second->getSources()) {
                if(ConnectionManagerPeerMatch::samePeer(peer, s.getUser()))
                    note(s.getUser().user);
            }
        }
    }
    for(const auto& u: users)
        removeSource(u, reason);
}

void QueueManager::remove(const string& aTarget) noexcept {
    UserList x;

    {
        Lock l(cs);

        QueueItem* q = fileQueue.find(aTarget);
        if(!q)
            return;

        if(q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD)) {
            auto dp = directories.equal_range(q->getSources()[0].getUser());
            for(auto i = dp.first; i != dp.second; ++i) {
                delete i->second;
            }
            directories.erase(q->getSources()[0].getUser());
        }

        if(q->isRunning()) {
            for(auto& i: q->getDownloads()) {
                x.push_back(i->getUser());
            }
        } else if(!q->getTempTarget().empty() && q->getTempTarget() != q->getTarget()) {
            File::deleteFile(q->getTempTarget());
        }

        fire(QueueManagerListener::Removed(), q);

        if(!q->isFinished()) {
            userQueue.remove(q);
        }
        fileQueue.remove(q);

        setDirty();
    }

    for(auto& i: x) {
        ConnectionManager::getInstance()->disconnect(i, true);
    }
}

int64_t QueueManager::getQueued(const UserPtr& aUser) const {
    Lock l(cs);
    return userQueue.getQueued(aUser);
}

void QueueManager::setPriority(const string& aTarget, QueueItem::Priority p) noexcept {
    HintedUserList getConn;
    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();

    {
        Lock l(cs);

        QueueItem* q = fileQueue.find(aTarget);
        if( (q != NULL) && (q->getPriority() != p) && !q->isFinished() ) {
            if(q->getPriority() == QueueItem::PAUSED || p == QueueItem::HIGHEST) {
                // Problem, we have to request connections to all these users...
                q->getOnlineUsers(getConn, queued);
            }
            userQueue.setPriority(q, p);
            setDirty();
            fire(QueueManagerListener::StatusUpdated(), q);
        }
    }

    for(auto& i: getConn) {
        ConnectionManager::getInstance()->getDownloadConnection(i);
    }
}

int QueueManager::countOnlineSources(const string& aTarget) {
    Lock l(cs);

    QueueItem* qi = fileQueue.find(aTarget);
    if(!qi)
        return 0;
    int onlineSources = 0;
    for(auto& i: qi->getSources()) {
        if(i.getUser().user->isOnline())
            onlineSources++;
    }
    return onlineSources;
}

} // namespace dcpp
