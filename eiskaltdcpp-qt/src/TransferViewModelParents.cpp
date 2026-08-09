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
#include "transferdisplay/TransferDisplay.h"
#include "transfersession/TransferGroup.h"

void TransferViewModel::updateParents(){
    for (const auto &i : rootItem->childItems) {
        if (i->childCount() < 1)
            continue;
        updateParent(i);
        const QModelIndex idx = createIndexForItem(i);
        if (idx.isValid()) {
            emit dataChanged(index(idx.row(), 0, idx.parent()),
                             index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
        }
    }

    // SPEED order changes every tick; other columns stay stable with dataChanged only.
    if (sortColumn == COLUMN_TRANSFER_SPEED)
        sort(sortColumn, sortOrder);

    pruneEmptyParents();
}

void TransferViewModel::updateParent(TransferViewItem *p){
    if (!p || p == rootItem)
        return;
    TransferGroup(p).refresh();
}

void TransferViewModel::updateTransferPos(const VarMap &params, qint64 pos){
    if (params.empty() || !params.contains("CID"))
        return;

    TransferViewItem *item;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item,
                      vbol(params["DOWN"]) ? QString() : vstr(params["HOST"])))
        return;
    if (item->finished)
        return;

    // Downloads: file position on the parent (GTK-style), not child segment dpos.
    if (vbol(params["DOWN"])) {
        TransferViewItem *group = item->parent();
        if (group && group != rootItem && rootItem->childItems.contains(group)) {
            group->dpos = TransferDisplay::highWaterBytes(group->dpos, pos);
            updateParent(group);
            const QModelIndex pidx = createIndexForItem(group);
            if (pidx.isValid()) {
                emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                                 index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
            }
        }
        return;
    }

    // Uploads use Tick/Starting (updateTransfer), not this signal.
    item->dpos = pos;
    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
}
