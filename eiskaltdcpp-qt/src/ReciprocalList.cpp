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

#include "dcpp/ListCache.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Transfer.h"
#include "dcpp/Upload.h"

using namespace dcpp;

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
    if (!item || item->isSet(QueueItem::FLAG_USER_LIST))
        return;
    maybeFetch(user);
}

void ReciprocalList::maybeFetch(const HintedUser &peer)
{
    if (!peer.user || !peer.user->isOnline())
        return;

    QueueManager *qm = QueueManager::getInstance();
    if (!qm || qm->hasListQueued(peer))
        return;

    const string listBase = qm->getListPath(peer);

    // Share size unchanged → addList reuses disk list (ListCached → ingest).
    // Share size changed → download at most once per day; always download if file missing.
    if (!ListCache::matchesUserShare(peer, listBase)
            && ListCache::fetchedWithinDay(listBase)
            && !ListCache::findListFile(listBase).empty())
        return;

    try {
        // flags 0: silent list (ShareIndex ingest, no ShareBrowser / Listings).
        qm->addList(peer, 0);
    } catch (const Exception &) {
    }
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
