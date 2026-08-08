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
#include "PeerConnectHub.h"
#include "PeerConnectLog.h"

namespace dcpp {

/** Add a source to an existing queue item */
bool QueueManager::addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad,
        const QueuedDownloadUsers* queuedPrefetched, bool notify, bool* updated) {
    bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && !userQueue.getRunning(aUser);

    // Silent/unreachable: block auto-search / ShareIndex re-attach. Explicit add/readd clears first.
    if(PeerConnectHub::isUnreachablePeer(aUser.user))
        return false;

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

    bool fireSourceAdded = false;
    if(aUser.user->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive() ) {
        PeerConnectLog::passiveSkip(aUser);
        qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
        wantConnection = false;
    } else if(qi->isFinished()) {
        wantConnection = false;
        fireSourceAdded = true;
    } else {
        userQueue.add(qi, aUser);
        fireSourceAdded = true;
    }

    setDirty();
    if(updated)
        *updated = true;

    if(!notify)
        return wantConnection;

    if(fireSourceAdded)
        fire(QueueManagerListener::SourceAdded(), qi, aUser);
    fire(QueueManagerListener::SourcesUpdated(), qi);

    // Prefer a snapshot taken before QueueManager::cs; never touch CM while QM is held.
    const QueuedDownloadUsers empty;
    const QueuedDownloadUsers& localQueued = queuedPrefetched ? *queuedPrefetched : empty;
    if(hasBusyAlias(qi, aUser, localQueued))
        wantConnection = false;
    return wantConnection;
}

} // namespace dcpp
