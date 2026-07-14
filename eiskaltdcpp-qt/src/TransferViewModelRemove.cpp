/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "TransferViewRemoveUtil.h"

using namespace TransferViewRemove;

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

    pruneEmptyParents();
}

bool TransferViewModel::shouldRemoveStaleRow(const TransferViewItem *item) const {
    return !keepAcrossReconnect(item, tr("Downloaded "), tr("Uploaded "));
}

void TransferViewModel::removeTransfer(const VarMap &params){
    if (params.empty() || vstr(params["CID"]).isEmpty())
        return;

    const QString cid = vstr(params["CID"]);
    const bool download = vbol(params["DOWN"]);
    const QString hub = download ? QString() : vstr(params["HOST"]);
    const QString dlPrefix = tr("Downloaded ");
    const QString ulPrefix = tr("Uploaded ");

    if (download) {
        TransferViewItem *item = nullptr;
        if (findTransfer(cid, true, &item)
                && keepAcrossReconnect(item, dlPrefix, ulPrefix)) {
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
    while (i != transfer_hash.end() && i.key() == cid) {
        TransferViewItem *item = i.value();
        if (!matchesRemove(item, download, hub)) {
            ++i;
            continue;
        }
        dropTransferRow(item);
        return;
    }

    if (!download && !hub.isEmpty()) {
        i = transfer_hash.find(cid);
        while (i != transfer_hash.end() && i.key() == cid) {
            TransferViewItem *item = i.value();
            if (!item->download) {
                dropTransferRow(item);
                return;
            }
            ++i;
        }
    }
}
