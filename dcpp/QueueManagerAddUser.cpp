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

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "File.h"
#include "PeerConnectHub.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "queue/LocalMatch.h"

#include <utility>
#include <vector>

namespace dcpp {

namespace {

void rejectSelfDownload(const HintedUser& user) {
    if(user == ClientManager::getInstance()->getMe())
        throw QueueException(_("You're trying to download from yourself!"));
}

void rejectSharedTth(const TTHValue& root) {
    if(BOOLSETTING(DONT_DL_ALREADY_SHARED) && ShareManager::getInstance()->isTTHShared(root))
        throw QueueException(_("A file with the same hash already exists in your share"));
}

void createEmptyFile(const string& target) {
    if(BOOLSETTING(SKIP_ZERO_BYTE))
        return;
    File::ensureDirectory(target);
    File f(target, File::WRITE, File::CREATE);
}

void noteSource(vector<pair<QueueItem*, bool>>& updates, QueueItem* qi, const HintedUser& user, bool updated) {
    if(updated)
        updates.emplace_back(qi, qi->isSource(user));
}

} // namespace

bool QueueManager::attachQueuedSources(const TTHValue& root, const HintedUser& user, int addBad,
        const QueuedDownloadUsers& queued, bool& wantConnection, SourceUpdates& updates) {
    if(!BOOLSETTING(DONT_DL_ALREADY_QUEUED))
        return false;

    auto ql = fileQueue.find(root);
    if(ql.empty())
        return false;

    bool sourceAdded = false;
    Flags::MaskType flags = addBad ? QueueItem::Source::FLAG_MASK : 0;
    for(auto& i: ql) {
        if(i->isSource(user))
            continue;
        try {
            bool updated = false;
            wantConnection = addSource(i, user, flags, &queued, false, &updated);
            noteSource(updates, i, user, updated);
            sourceAdded = true;
        } catch(...) { }
    }
    if(!sourceAdded)
        throw QueueException(_("This file is already queued"));
    return true;
}

QueueItem* QueueManager::queueFileItem(const string& target, int64_t size, int flags,
        const string& tempTarget, const TTHValue& root, vector<QueueItem*>& added) {
    QueueItem* q = fileQueue.find(target);
    if(!q) {
        q = fileQueue.add(target, size, flags, QueueItem::DEFAULT, tempTarget, GET_TIME(), root);
        added.push_back(q);
        return q;
    }
    if(q->getSize() != size)
        throw QueueException(_("A file with a different size already exists in the queue"));
    if(!(root == q->getTTH()))
        throw QueueException(_("A file with a different TTH root already exists in the queue"));
    if(q->isFinished())
        throw QueueException(_("This file has already finished downloading"));
    q->setFlag(flags);
    return q;
}

void QueueManager::notifyQueuedAdd(const HintedUser& user, bool wantConnection,
        const QueuedDownloadUsers& queued, const vector<QueueItem*>& added, SourceUpdates& updates) {
    for(auto* q: added)
        fire(QueueManagerListener::Added(), q);
    for(auto& u: updates) {
        if(u.second)
            fire(QueueManagerListener::SourceAdded(), u.first, user);
        fire(QueueManagerListener::SourcesUpdated(), u.first);
        if(wantConnection && hasBusyAlias(u.first, user, queued))
            wantConnection = false;
    }
    if(wantConnection && user.user->isOnline())
        ConnectionManager::getInstance()->getDownloadConnection(user);
}

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
                       int aFlags /* = 0 */, bool addBad /* = true */)
{
    rejectSelfDownload(aUser);
    PeerConnectHub::clearUnreachablePeer(aUser.user);
    rejectSharedTth(root);

    string target;
    string tempTarget;
    const bool userList = (aFlags & QueueItem::FLAG_USER_LIST) == QueueItem::FLAG_USER_LIST;
    if(userList) {
        target = getListPath(aUser);
        tempTarget = aTarget;
    } else {
        target = checkTarget(aTarget, false);
        if(LocalMatch::matches(target, aSize, root))
            return;
        target = checkTarget(aTarget, true);
    }

    // True empty files (no TTH): create locally. Hashed size-0 hits still queue.
    if(aSize == 0 && !root) {
        createEmptyFile(target);
        return;
    }

    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();
    vector<QueueItem*> addedItems;
    SourceUpdates sourceUpdates;
    bool wantConnection = true;
    {
        Lock l(cs);
        if(!userList && attachQueuedSources(root, aUser, addBad, queued, wantConnection, sourceUpdates)) {
            // attached to existing TTH — notify below
        } else {
            QueueItem* q = queueFileItem(target, aSize, aFlags, tempTarget, root, addedItems);
            bool updated = false;
            wantConnection = addSource(q, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0,
                                       &queued, false, &updated);
            noteSource(sourceUpdates, q, aUser, updated);
        }
    }
    notifyQueuedAdd(aUser, wantConnection, queued, addedItems, sourceUpdates);
}

void QueueManager::readd(const string& target, const HintedUser& aUser) {
    bool wantConnection = false;
    PeerConnectHub::clearUnreachablePeer(aUser.user);
    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();
    QueueItem* updatedQi = nullptr;
    bool fireSourceAdded = false;
    {
        Lock l(cs);
        QueueItem* q = fileQueue.find(target);
        if(q && q->isBadSource(aUser)) {
            bool updated = false;
            wantConnection = addSource(q, aUser, QueueItem::Source::FLAG_MASK, &queued, false, &updated);
            if(updated) {
                updatedQi = q;
                fireSourceAdded = q->isSource(aUser);
            }
        }
    }
    if(updatedQi) {
        if(fireSourceAdded)
            fire(QueueManagerListener::SourceAdded(), updatedQi, aUser);
        fire(QueueManagerListener::SourcesUpdated(), updatedQi);
        if(wantConnection && hasBusyAlias(updatedQi, aUser, queued))
            wantConnection = false;
    }
    if(wantConnection && aUser.user->isOnline())
        ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

} // namespace dcpp
