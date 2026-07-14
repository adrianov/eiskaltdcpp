/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewRemoveUtil.h"
#include "TransferDisplay.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/QueueItem.h"
#include "dcpp/QueueManager.h"

using namespace dcpp;

namespace TransferViewRemove {

void eraseHashEntry(QMultiHash<QString, TransferViewItem*> &hash, TransferViewItem *item) {
    if (!item || item->cid.isEmpty())
        return;

    auto i = hash.find(item->cid);
    while (i != hash.end() && i.key() == item->cid) {
        if (i.value() == item) {
            hash.erase(i);
            return;
        }
        ++i;
    }
}

bool offlineOrphan(const QString &cid, const QString &host) {
    if (cid.isEmpty() || !host.isEmpty())
        return false;
    const UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    return user && !user->isOnline()
            && ClientManager::getInstance()->resolveHubHint(user).empty();
}

namespace {

bool stillActiveForTarget(const QString &cid, const QString &target) {
    if (cid.isEmpty() || target.isEmpty())
        return false;
    const UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    if (!user || QueueManager::getInstance()->hasDownload(user) == QueueItem::PAUSED)
        return false;
    if (!ConnectionManager::getInstance()->isQueuedForDownload(user))
        return false;

    string aTarget;
    int64_t aSize = 0;
    int aFlags = 0;
    if (!QueueManager::getInstance()->getQueueInfo(user, aTarget, aSize, aFlags))
        return false;
    return _q(aTarget) == target;
}

} // namespace

bool keepAcrossReconnect(const TransferViewItem *item, const QString &dl, const QString &ul) {
    // Uploads never linger across fail/remove; only download progress for the same file.
    if (!item || !item->download
            || offlineOrphan(item->cid, item->data(COLUMN_TRANSFER_HOST).toString()))
        return false;
    if (!(item->dpos > 0 || item->percent > 0
            || TransferDisplay::isProgressStat(item->data(COLUMN_TRANSFER_STATS).toString(), dl, ul)))
        return false;
    return stillActiveForTarget(item->cid, item->target);
}

bool matchesRemove(const TransferViewItem *item, bool download, const QString &hub) {
    if (item->download != download)
        return false;
    if (download || hub.isEmpty())
        return true;
    return item->data(COLUMN_TRANSFER_HOST).toString() == hub;
}

} // namespace TransferViewRemove
