/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ReciprocalList.h"

#ifdef USE_QT_SQLITE

#include "ReciprocalListPeer.h"

#include "dcpp/ConnectionManagerPeerMatch.h"
#include "dcpp/ListCache.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Transfer.h"
#include "dcpp/Upload.h"
#include "dcpp/User.h"

#include <ctime>
#include <vector>

using namespace dcpp;

namespace {

/** Cap silent list downloads so reciprocal fetch cannot flood the queue. */
const size_t kMaxQueuedLists = 3;

void queueSilentList(QueueManager *qm, const HintedUser &peer)
{
    try {
        qm->addList(peer, 0);
    } catch (const Exception &) {
    }
}

} // namespace

ReciprocalList::~ReciprocalList()
{
    stop();
}

void ReciprocalList::start()
{
    UploadManager::getInstance()->addListener(this);
    QueueManager::getInstance()->addListener(this);
    TimerManager::getInstance()->addListener(this);
}

void ReciprocalList::stop()
{
    if (TimerManager::getInstance())
        TimerManager::getInstance()->removeListener(this);
    if (QueueManager::getInstance())
        QueueManager::getInstance()->removeListener(this);
    if (UploadManager::getInstance())
        UploadManager::getInstance()->removeListener(this);
}

void ReciprocalList::queueFetch(const HintedUser &peer)
{
    if (!peer.user)
        return;
    Lock l(pendingCs);
    for (const auto &p : pending) {
        if (!p.user)
            continue;
        if (p.user->getCID() == peer.user->getCID())
            return;
        if (ConnectionManagerPeerMatch::samePeer(p, peer))
            return;
    }
    pending.push_back(peer);
}

void ReciprocalList::on(UploadManagerListener::Complete, Upload *ul) noexcept
{
    if (!ul || ul->getType() != Transfer::TYPE_FULL_LIST)
        return;
    queueFetch(ul->getHintedUser());
}

void ReciprocalList::on(QueueManagerListener::SourceAdded, QueueItem *item,
                        const HintedUser &user) noexcept
{
    if (!item || item->isSet(QueueItem::FLAG_USER_LIST) || item->isFinished())
        return;
    queueFetch(user);
}

void ReciprocalList::on(TimerManagerListener::Second, uint64_t) noexcept
{
    vector<HintedUser> work;
    {
        Lock l(pendingCs);
        work.swap(pending);
    }
    for (const auto &peer : work)
        maybeFetch(peer);
}

void ReciprocalList::maybeFetch(const HintedUser &peer)
{
    if (!peer.user || !peer.user->isOnline() || peer.user->isSet(User::VIRUS_INFECTED))
        return;
    if (ReciprocalListPeer::cooldownActive(peer))
        return;

    QueueManager *qm = QueueManager::getInstance();
    if (!qm || qm->hasListQueued(peer))
        return;

    HintedUser cachePeer = peer;
    string listBase;
    string listFile;
    const bool cached = ReciprocalListPeer::findCachedList(peer, cachePeer, listBase, listFile);

    if (cached) {
        ReciprocalListPeer::markChecked(peer);
        if (listFile.empty())
            return;
        queueSilentList(qm, peer);
        return;
    }

    if (!listFile.empty() && ListCache::fetchedWithinDay(peer)) {
        ReciprocalListPeer::markChecked(peer);
        return;
    }

    if (qm->countQueuedLists() >= kMaxQueuedLists)
        return;

    ReciprocalListPeer::markChecked(peer);
    queueSilentList(qm, peer);
}

#else

ReciprocalList::~ReciprocalList() {}
void ReciprocalList::start() {}
void ReciprocalList::stop() {}
void ReciprocalList::on(dcpp::UploadManagerListener::Complete, dcpp::Upload *) noexcept {}
void ReciprocalList::on(dcpp::QueueManagerListener::SourceAdded, dcpp::QueueItem *,
                        const dcpp::HintedUser &) noexcept {}
void ReciprocalList::on(dcpp::TimerManagerListener::Second, uint64_t) noexcept {}
void ReciprocalList::queueFetch(const dcpp::HintedUser &) {}
void ReciprocalList::maybeFetch(const dcpp::HintedUser &) {}

#endif
