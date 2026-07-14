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

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "PeerConnectLog.h"

namespace dcpp {

/** Add a source to an existing queue item */
bool QueueManager::addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad,
        const QueuedDownloadUsers* queuedPrefetched) {
    bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && !userQueue.getRunning(aUser);

    if(qi->isSource(aUser)) {
        if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
            return wantConnection;
        }
        throw QueueException(str(F_("Duplicate source: %1%") % Util::getFileName(qi->getTarget())));
    }

    if(qi->isBadSourceExcept(aUser, addBad)) {
        throw QueueException(str(F_("Duplicate source: %1%") % Util::getFileName(qi->getTarget())));
    }

    qi->addSource(aUser);

    if(aUser.user->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive() ) {
        PeerConnectLog::passiveSkip(aUser);
        qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
        wantConnection = false;
    } else if(qi->isFinished()) {
        wantConnection = false;
        fire(QueueManagerListener::SourceAdded(), qi, aUser);
    } else {
        userQueue.add(qi, aUser);
        fire(QueueManagerListener::SourceAdded(), qi, aUser);
    }

    fire(QueueManagerListener::SourcesUpdated(), qi);
    setDirty();

    // Prefer a snapshot taken before QueueManager::cs; never touch CM while QM is held.
    const QueuedDownloadUsers empty;
    const QueuedDownloadUsers& localQueued = queuedPrefetched ? *queuedPrefetched : empty;
    if(hasBusyAlias(qi, aUser, localQueued))
        wantConnection = false;
    return wantConnection;
}

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
    // @todo remove from finished items
    bool isRunning = false;
    string removeRunning;
    {
        Lock l(cs);
        QueueItem* qi = NULL;
        while( (qi = userQueue.getNext(aUser, QueueItem::PAUSED)) != NULL) {
            if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
                remove(qi->getTarget());
            } else {
                userQueue.remove(qi, aUser);
                qi->removeSource(aUser, reason);
                fire(QueueManagerListener::SourceRemoved(), qi, aUser, reason);
                fire(QueueManagerListener::SourcesUpdated(), qi);
                setDirty();
            }
        }

        qi = userQueue.getRunning(aUser);
        if(qi) {
            if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
                removeRunning = qi->getTarget();
            } else {
                userQueue.removeDownload(qi, aUser);
                userQueue.remove(qi, aUser);
                isRunning = true;
                qi->removeSource(aUser, reason);
                fire(QueueManagerListener::SourceRemoved(), qi, aUser, reason);
                fire(QueueManagerListener::StatusUpdated(), qi);
                fire(QueueManagerListener::SourcesUpdated(), qi);
                setDirty();
            }
        }
    }

    if(isRunning) {
        ConnectionManager::getInstance()->disconnect(aUser, true);
    }
    if(!removeRunning.empty()) {
        remove(removeRunning);
    }
}

} // namespace dcpp
