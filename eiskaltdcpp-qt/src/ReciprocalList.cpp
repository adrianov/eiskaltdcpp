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

#include "ShareIndex.h"

#include "dcpp/ListCache.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Transfer.h"
#include "dcpp/Upload.h"
#include "dcpp/User.h"

#include <QMutex>
#include <QMutexLocker>
#include <QString>

#include <ctime>
#include <unordered_map>

using namespace dcpp;

namespace {

/** Cap silent list downloads so reciprocal fetch cannot flood the queue. */
const size_t kMaxQueuedLists = 3;
/** Per-peer pause after a reciprocal check (cache hit, skip, or queue). */
const time_t kPeerCooldownSecs = 10 * 60;

QMutex gPeerMutex;
std::unordered_map<string, time_t> gPeerChecked;

bool peerCooldownActive(const CID &cid)
{
    const string key = cid.toBase32();
    QMutexLocker lock(&gPeerMutex);
    const auto it = gPeerChecked.find(key);
    if (it == gPeerChecked.end())
        return false;
    return (time(nullptr) - it->second) < kPeerCooldownSecs;
}

void markPeerChecked(const CID &cid)
{
    const time_t now = time(nullptr);
    QMutexLocker lock(&gPeerMutex);
    if (gPeerChecked.size() > 512) {
        for (auto it = gPeerChecked.begin(); it != gPeerChecked.end(); ) {
            if ((now - it->second) >= kPeerCooldownSecs)
                it = gPeerChecked.erase(it);
            else
                ++it;
        }
    }
    gPeerChecked[cid.toBase32()] = now;
}

bool needsIngest(const UserPtr &user, const string &listFile)
{
    if (!user || listFile.empty() || !ShareIndex::getInstance())
        return false;
    return ShareIndex::getInstance()->needsListIngest(
        QString::fromStdString(user->getCID().toBase32()),
        QString::fromStdString(listFile));
}

void queueSilentList(QueueManager *qm, const HintedUser &peer)
{
    try {
        // flags 0: silent list (ShareIndex ingest, no ShareBrowser).
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
}

void ReciprocalList::stop()
{
    if (QueueManager::getInstance())
        QueueManager::getInstance()->removeListener(this);
    if (UploadManager::getInstance())
        UploadManager::getInstance()->removeListener(this);
}

void ReciprocalList::on(UploadManagerListener::Complete, Upload *ul) noexcept
{
    if (!ul || ul->getType() != Transfer::TYPE_FULL_LIST)
        return;
    maybeFetch(ul->getHintedUser());
}

void ReciprocalList::on(QueueManagerListener::SourceAdded, QueueItem *item,
                        const HintedUser &user) noexcept
{
    // Finished items still fire SourceAdded when alternate sources appear; skip those.
    if (!item || item->isSet(QueueItem::FLAG_USER_LIST) || item->isFinished())
        return;
    maybeFetch(user);
}

void ReciprocalList::maybeFetch(const HintedUser &peer)
{
    if (!peer.user || !peer.user->isOnline() || peer.user->isSet(User::VIRUS_INFECTED))
        return;
    if (peerCooldownActive(peer.user->getCID()))
        return;

    QueueManager *qm = QueueManager::getInstance();
    if (!qm || qm->hasListQueued(peer))
        return;

    const string listBase = qm->getListPath(peer);
    const string listFile = ListCache::findListFile(listBase);

    // Share size unchanged: reuse disk list. Only touch the queue when ShareIndex is stale.
    if (ListCache::matchesUserShare(peer, listBase)) {
        markPeerChecked(peer.user->getCID());
        if (!needsIngest(peer.user, listFile))
            return;
        queueSilentList(qm, peer);
        return;
    }

    // Share size changed (or no .sharesize): refresh at most once per day if a list exists.
    if (!listFile.empty() && ListCache::fetchedWithinDay(listBase)) {
        markPeerChecked(peer.user->getCID());
        return;
    }

    if (qm->countQueuedLists() >= kMaxQueuedLists)
        return;

    markPeerChecked(peer.user->getCID());
    queueSilentList(qm, peer);
}

#else

ReciprocalList::~ReciprocalList() {}
void ReciprocalList::start() {}
void ReciprocalList::stop() {}
void ReciprocalList::on(dcpp::UploadManagerListener::Complete, dcpp::Upload *) noexcept {}
void ReciprocalList::on(dcpp::QueueManagerListener::SourceAdded, dcpp::QueueItem *,
                        const dcpp::HintedUser &) noexcept {}
void ReciprocalList::maybeFetch(const dcpp::HintedUser &) {}

#endif
