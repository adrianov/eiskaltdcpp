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
#include "transfersession/TransferSession.h"

using namespace TransferViewRemove;

void TransferViewModel::releaseEmptyGroup(TransferViewItem *group) {
    if (!group || group == rootItem || !rootItem->childItems.contains(group))
        return;
    if (group->childCount() > 0) {
        updateParent(group);
        notifyTransferChange(group);
        return;
    }
    if (group->holdFinished())
        return;
    beginRemoveRows(QModelIndex(), group->row(), group->row());
    rootItem->childItems.removeAt(group->row());
    delete group;
    endRemoveRows();
}

void TransferViewModel::dropTransferRow(TransferViewItem *item) {
    if (!item)
        return;

    TransferViewItem *p = item->parent();
    if (p && p->childItems.contains(item)) {
        beginRemoveRows(createIndexForItem(p), item->row(), item->row());
        p->childItems.removeAt(item->row());
        endRemoveRows();
    }

    auto i = transfer_hash.find(item->cid);
    while (i != transfer_hash.end() && i.key() == item->cid) {
        if (i.value() == item) {
            transfer_hash.erase(i);
            break;
        }
        ++i;
    }
    delete item;
    releaseEmptyGroup(p);
    pruneEmptyParents();
}

bool TransferViewModel::shouldRemoveStaleRow(const TransferViewItem *item) const {
    return !keepAcrossReconnect(item, tr("Downloaded "), tr("Uploaded "));
}

bool TransferViewModel::parkDownloadReconnect(const QString &cid) {
    TransferViewItem *item = nullptr;
    if (!findTransfer(cid, true, &item)
            || !keepAcrossReconnect(item, tr("Downloaded "), tr("Uploaded ")))
        return false;

    item->fail = false;
    item->finished = false;
    item->updateColumn(COLUMN_TRANSFER_SPEED, 0);
    notifyTransferChange(item);
    return true;
}

bool TransferViewModel::dropTransferByCid(const QString &cid, bool download, const QString &hub) {
    auto i = transfer_hash.find(cid);
    while (i != transfer_hash.end() && i.key() == cid) {
        TransferViewItem *item = i.value();
        if (!matchesRemove(item, download, hub)) {
            ++i;
            continue;
        }
        if (!download)
            grace.cancelUpload(cid, hub);
        dropTransferRow(item);
        return true;
    }
    return false;
}

void TransferViewModel::dropLoneUpload(const QString &cid, const QString &hub) {
    TransferViewItem *only = nullptr;
    int uploads = 0;
    auto i = transfer_hash.find(cid);
    while (i != transfer_hash.end() && i.key() == cid) {
        if (!i.value()->download) {
            only = i.value();
            ++uploads;
        }
        ++i;
    }
    if (uploads == 1) {
        grace.cancelUpload(cid, hub);
        dropTransferRow(only);
    }
}

void TransferViewModel::removeTransfer(const VarMap &params){
    if (params.empty() || vstr(params["CID"]).isEmpty())
        return;

    const QString cid = vstr(params["CID"]);
    const bool download = vbol(params["DOWN"]);
    const QString hub = download ? QString() : vstr(params["HOST"]);

    if (download && parkDownloadReconnect(cid))
        return;
    if (dropTransferByCid(cid, download, hub))
        return;
    // Host mismatch: drop only when this CID has one upload.
    if (!download && !hub.isEmpty())
        dropLoneUpload(cid, hub);
}

void TransferViewModel::pruneUpload(VarMap params) {
    TransferViewItem *item = findUploadRow(params);
    if (!item || item->download)
        return;

    TransferSession session(TransferSession::scopeOf(item, rootItem));
    TransferViewItem *scope = session.item();
    if (scope && scope->cid.isEmpty() && session.uploadDone()) {
        const QList<TransferViewItem*> children = scope->childItems;
        for (TransferViewItem *child : children)
            dropTransferRow(child);
        return;
    }
    dropTransferRow(item);
}
