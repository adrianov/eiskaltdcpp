/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ConnectionManagerPeerMatch.h"
#include "ListCache.h"
#include "PeerConnectLog.h"

namespace dcpp {

void QueueManager::purgeOtherListQueues(const HintedUser& aUser) {
    StringList stale;

    {
        Lock l(cs);
        for(auto& i: fileQueue.getQueue()) {
            QueueItem* qi = i.second;
            if(!qi->isSet(QueueItem::FLAG_USER_LIST) || qi->isFinished() || qi->getSources().empty())
                continue;

            const HintedUser& src = qi->getSources()[0].getUser();
            if(src.user == aUser.user && src.hint == aUser.hint)
                continue;

            if(ConnectionManagerPeerMatch::samePeer(aUser, src))
                stale.push_back(qi->getTarget());
        }
    }

    for(auto& target: stale)
        remove(target);
}

bool QueueManager::hasListQueued(const HintedUser& user) noexcept {
    if(!user.user)
        return false;
    Lock l(cs);
    QueueItem* qi = fileQueue.find(getListPath(user));
    if(qi && qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isFinished())
        return true;
    for(const auto& i: fileQueue.getQueue()) {
        if(!i.second->isSet(QueueItem::FLAG_USER_LIST) || i.second->isFinished()
                || i.second->getSources().empty())
            continue;
        const HintedUser& src = i.second->getSources()[0].getUser();
        if(ConnectionManagerPeerMatch::samePeer(user, src))
            return true;
    }
    return false;
}

bool QueueManager::tryUseCachedList(const HintedUser& aUser, int aFlags, const string& aInitialDir) {
    if(tryUseCachedListAt(aUser, aFlags, aInitialDir, aUser, getListPath(aUser)))
        return true;

    bool used = false;
    ConnectionManagerPeerMatch::forEachListPeer(aUser, [&](const HintedUser& alias) {
        if(used || alias.user == aUser.user)
            return;
        if(tryUseCachedListAt(aUser, aFlags, aInitialDir, alias, getListPath(alias)))
            used = true;
    });
    return used;
}

bool QueueManager::tryUseCachedListAt(const HintedUser& aUser, int aFlags, const string& aInitialDir,
        const HintedUser& cacheUser, const string& listBase) {
    if(!ListCache::matchesUserShare(cacheUser, listBase))
        return false;

    const string listFile = ListCache::findListFile(listBase);
    const int processFlags = aFlags & (QueueItem::FLAG_DIRECTORY_DOWNLOAD | QueueItem::FLAG_MATCH_QUEUE);
    if(processFlags)
        processList(listFile, aUser, processFlags);

    if(aFlags & QueueItem::FLAG_CLIENT_VIEW) {
        PeerConnectLog::cachedList(aUser, listFile);
        fire(QueueManagerListener::ListFromCache(), aUser, listFile, aInitialDir);
        return true;
    }

    fire(QueueManagerListener::ListCached(), aUser, listFile);
    return true;
}

} // namespace dcpp
