/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ConnectionManager.h"
#include "PeerConnectHub.h"

namespace dcpp {

vector<TTHValue> QueueManager::getQueuedTTHs() noexcept {
    unordered_set<TTHValue> roots;
    Lock l(cs);
    for(const auto& item: fileQueue.getQueue()) {
        QueueItem* qi = item.second;
        if(!qi->isFinished() && !qi->isSet(QueueItem::FLAG_USER_LIST))
            roots.insert(qi->getTTH());
    }
    return vector<TTHValue>(roots.begin(), roots.end());
}

void QueueManager::matchSources(const HintedUser& user,
                                const vector<SourceMatch>& matches) noexcept {
    if(!user.user || matches.empty())
        return;

    unordered_map<TTHValue, int64_t> indexed;
    for(const auto& match: matches)
        indexed.emplace(match.tth, match.size);

    bool wantConnection = false;
    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();
    {
        Lock l(cs);
        for(const auto& item: fileQueue.getQueue()) {
            QueueItem* qi = item.second;
            const auto match = indexed.find(qi->getTTH());
            if(qi->isFinished() || qi->isSet(QueueItem::FLAG_USER_LIST)
                    || match == indexed.end() || match->second != qi->getSize())
                continue;
            if(qi->isSource(user)) {
                // Same as search hits: already a source still needs a connect nudge
                // (revives given-up CQIs / idle sockets).
                wantConnection |= !userQueue.getRunning(user.user)
                        && shouldConnectSource(qi, user, queued);
                continue;
            }
            try {
                wantConnection |= addSource(qi, user, QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, &queued);
            } catch(const Exception&) { }
        }
    }

    if(wantConnection && user.user->isOnline())
        ConnectionManager::getInstance()->getDownloadConnection(user);
}

void QueueManager::removeSource(const string& aTarget, const UserPtr& aUser, int reason, bool removeConn /* = true */) noexcept {
    bool isRunning = false;
    bool removeCompletely = false;
    {
        Lock l(cs);
        QueueItem* q = fileQueue.find(aTarget);
        if(!q)
            return;

        if(!q->isSource(aUser))
            return;

        if(q->isSet(QueueItem::FLAG_USER_LIST)) {
            removeCompletely = true;
            goto endCheck;
        }

        if(q->isRunning() && userQueue.getRunning(aUser) == q) {
            isRunning = true;
            userQueue.removeDownload(q, aUser);
            fire(QueueManagerListener::StatusUpdated(), q);
        }

        if(!q->isFinished()) {
            userQueue.remove(q, aUser);
        }
        q->removeSource(aUser, reason);

        fire(QueueManagerListener::SourceRemoved(), q, aUser, reason);
        fire(QueueManagerListener::SourcesUpdated(), q);
        setDirty();
    }
endCheck:
    if(isRunning && removeConn) {
        ConnectionManager::getInstance()->disconnect(aUser, true);
    }
    if(removeCompletely) {
        remove(aTarget);
    }
}

void QueueManager::removeSource(const UserPtr& aUser, int reason) noexcept {
    // Walk the full file queue — userQueue.getNext() skips items that are not
    // download-eligible (e.g. already running from another source).
    if(reason == QueueItem::Source::FLAG_UNREACHABLE)
        PeerConnectHub::noteUnreachablePeer(aUser);

    bool isRunning = false;
    StringList listsToRemove;
    {
        Lock l(cs);
        for(const auto& item: fileQueue.getQueue()) {
            QueueItem* qi = item.second;
            if(!qi->isSource(aUser))
                continue;

            if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
                listsToRemove.push_back(qi->getTarget());
                continue;
            }

            if(userQueue.getRunning(aUser) == qi) {
                userQueue.removeDownload(qi, aUser);
                isRunning = true;
                fire(QueueManagerListener::StatusUpdated(), qi);
            }
            if(!qi->isFinished())
                userQueue.remove(qi, aUser);
            qi->removeSource(aUser, reason);
            // Unreachable uses PeerUnreachable once after the walk (not per-item SourceRemoved).
            if(reason != QueueItem::Source::FLAG_UNREACHABLE)
                fire(QueueManagerListener::SourceRemoved(), qi, aUser, reason);
            fire(QueueManagerListener::SourcesUpdated(), qi);
            setDirty();
        }
    }

    if(isRunning)
        ConnectionManager::getInstance()->disconnect(aUser, true);
    for(const auto& target: listsToRemove)
        remove(target);

    if(reason == QueueItem::Source::FLAG_UNREACHABLE)
        fire(QueueManagerListener::PeerUnreachable(), aUser);
}

} // namespace dcpp
