/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "TransferDisplay.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/QueueItem.h"
#include "dcpp/QueueManager.h"

using namespace dcpp;

namespace {

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

bool moreDownloadsFrom(const QString &cid) {
    if (cid.isEmpty())
        return false;
    const UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    if (!user)
        return false;
    return QueueManager::getInstance()->hasDownload(user) != QueueItem::PAUSED;
}

// Progress rows may linger across reconnect; Connecting/Connected must not.
bool keepAcrossReconnect(const TransferViewItem *item, const QString &dl, const QString &ul) {
    return item && (item->dpos > 0 || item->percent > 0
            || TransferDisplay::isProgressStat(item->data(COLUMN_TRANSFER_STATS).toString(), dl, ul));
}

} // namespace

void TransferViewModel::removeTransfer(const VarMap &params){
    if (params.empty() || vstr(params["CID"]).isEmpty())
        return;

    const QString cid = vstr(params["CID"]);
    const bool download = vbol(params["DOWN"]);
    const QString hub = download ? QString() : vstr(params["HOST"]);

    if (download && moreDownloadsFrom(cid)) {
        TransferViewItem *item = nullptr;
        if (findTransfer(cid, true, &item)
                && keepAcrossReconnect(item, tr("Downloaded "), tr("Uploaded "))) {
            item->fail = false;
            item->finished = false;
            item->updateColumn(COLUMN_TRANSFER_SPEED, 0);
            const QModelIndex idx = createIndexForItem(item);
            if (idx.isValid()) {
                emit dataChanged(index(idx.row(), 0, idx.parent()),
                                 index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
            }
            return;
        }
    }

    auto i = transfer_hash.find(cid);
    while (i != transfer_hash.end() && i.key() == cid){
        TransferViewItem *item = i.value();
        if (item->download == download
                && (item->download || hub.isEmpty()
                    || vstr(item->data(COLUMN_TRANSFER_HOST)) == hub)){
            TransferViewItem *p = item->parent();

            if (p && p->childItems.contains(item)) {
                beginRemoveRows(createIndexForItem(p), item->row(), item->row());
                p->childItems.removeAt(item->row());
                endRemoveRows();
            }

            transfer_hash.erase(i);
            delete item;

            if (p && p != rootItem && rootItem->childItems.contains(p)) {
                if (!p->childCount()) {
                    beginRemoveRows(QModelIndex(), p->row(), p->row());
                    rootItem->childItems.removeAt(p->row());
                    delete p;
                    endRemoveRows();
                } else {
                    updateParent(p);
                    const QModelIndex pidx = createIndexForItem(p);
                    if (pidx.isValid()) {
                        emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                                         index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
                    }
                }
            }

            return;
        }

        ++i;
    }
}

void TransferViewModel::removeQueueTarget(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    const QString target = vstr(params["TARGET"]);
    if (target.isEmpty())
        return;

    // Defer so next-file Requesting can reparent before the group is destroyed.
    pendingTargetRemoves.insert(target);
    if (flushTargetsQueued)
        return;
    flushTargetsQueued = true;
    QMetaObject::invokeMethod(this, "flushPendingTargetRemoves", Qt::QueuedConnection);
}

void TransferViewModel::flushPendingTargetRemoves(){
    flushTargetsQueued = false;
    const QSet<QString> targets = pendingTargetRemoves;
    pendingTargetRemoves.clear();
    for (const QString &target : targets)
        removeQueueTargetNow(target);
}

void TransferViewModel::removeQueueTargetNow(const QString &target){
    if (target.isEmpty())
        return;

    const QString dlPrefix = tr("Downloaded ");
    const QString ulPrefix = tr("Uploaded ");

    TransferViewItem *p = nullptr;
    if (findParent(target, &p)) {
        while (p->childCount() > 0) {
            TransferViewItem *child = p->childItems.last();
            if (child->download && moreDownloadsFrom(child->cid)
                    && keepAcrossReconnect(child, dlPrefix, ulPrefix)) {
                moveTransfer(child, p, rootItem);
                child->finished = false;
                continue;
            }

            eraseHashEntry(transfer_hash, child);
            const int row = p->childCount() - 1;
            beginRemoveRows(createIndexForItem(p), row, row);
            p->childItems.removeLast();
            delete child;
            endRemoveRows();
        }

        if (rootItem->childItems.contains(p)) {
            const int parentRow = p->row();
            beginRemoveRows(QModelIndex(), parentRow, parentRow);
            rootItem->childItems.removeAt(parentRow);
            delete p;
            endRemoveRows();
        }
    }

    for (int row = rootItem->childCount() - 1; row >= 0; --row) {
        TransferViewItem *item = rootItem->child(row);
        if (!item->download || item->target != target)
            continue;
        if (!item->cid.isEmpty() && moreDownloadsFrom(item->cid)
                && keepAcrossReconnect(item, dlPrefix, ulPrefix)) {
            item->finished = false;
            continue;
        }

        eraseHashEntry(transfer_hash, item);
        beginRemoveRows(QModelIndex(), row, row);
        rootItem->childItems.removeAt(row);
        delete item;
        endRemoveRows();
    }
}
