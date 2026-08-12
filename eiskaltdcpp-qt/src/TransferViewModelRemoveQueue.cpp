/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "transfergrace/TransferViewRemoveUtil.h"

using namespace TransferViewRemove;

namespace {
/** Keep Finished downloads in Transfers so Open file works right after a quick grab. */
constexpr int finishedDownloadPruneMs = 60000;
}

void TransferViewModel::removeQueueTarget(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    const QString target = vstr(params["TARGET"]);
    if (target.isEmpty())
        return;

    pendingTargetRemoves.insert(target);
    if (flushTargetsQueued)
        return;
    flushTargetsQueued = true;
    QMetaObject::invokeMethod(this, "flushPendingTargetRemoves", Qt::QueuedConnection);
}

void TransferViewModel::dropQueueTarget(const QString &target){
    if (target.isEmpty())
        return;
    pendingTargetRemoves.remove(target);
    removeQueueTargetNow(target);
}

void TransferViewModel::flushPendingTargetRemoves(){
    flushTargetsQueued = false;
    const QSet<QString> targets = pendingTargetRemoves;
    pendingTargetRemoves.clear();
    for (const QString &target : targets) {
        TransferViewItem *p = nullptr;
        // Finished + queue Removed: hold the group so the user can open the file.
        if (findParent(target, &p) && p->finished) {
            grace.armDownload(target, finishedDownloadPruneMs);
            continue;
        }
        removeQueueTargetNow(target);
    }
}

void TransferViewModel::pruneDownload(QString target) {
    removeQueueTargetNow(target);
}

void TransferViewModel::removeQueueTargetNow(const QString &target){
    if (target.isEmpty())
        return;

    grace.cancelDownload(target);

    const QString dlPrefix = tr("Downloaded ");
    const QString ulPrefix = tr("Uploaded ");

    TransferViewItem *p = nullptr;
    if (findParent(target, &p)) {
        while (p->childCount() > 0) {
            TransferViewItem *child = p->childItems.last();
            if (child->download && keepAcrossReconnect(child, dlPrefix, ulPrefix)) {
                moveTransfer(child, p, rootItem);
                child->finished = false;
                continue;
            }

            eraseHashEntry(transfer_hash, child);
            const int row = p->childCount() - 1;
            beginRemoveRows(createIndexForItem(p), row, row);
            p->childItems.removeLast();
            destroyRow(child);
            endRemoveRows();
        }

        if (rootItem->childItems.contains(p)) {
            const int parentRow = p->row();
            beginRemoveRows(QModelIndex(), parentRow, parentRow);
            rootItem->childItems.removeAt(parentRow);
            destroyRow(p);
            endRemoveRows();
        }
    }

    for (int row = rootItem->childCount() - 1; row >= 0; --row) {
        TransferViewItem *item = rootItem->child(row);
        if (!item->download || item->target != target)
            continue;
        if (!item->cid.isEmpty() && keepAcrossReconnect(item, dlPrefix, ulPrefix)) {
            item->finished = false;
            continue;
        }

        eraseHashEntry(transfer_hash, item);
        beginRemoveRows(QModelIndex(), row, row);
        rootItem->childItems.removeAt(row);
        destroyRow(item);
        endRemoveRows();
    }

    pruneEmptyParents();
}
