/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

void TransferViewModel::finishParent(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    QString target = vstr(params["TARGET"]);
    TransferViewItem *p;

    if (!findParent(target, &p))
        return;

    // Only the group row — children may immediately start the next file.
    p->updateColumn(COLUMN_TRANSFER_STATS, tr("Finished"));
    p->percent = 100.0;
    p->finished = true;
    p->updateColumn(COLUMN_TRANSFER_SPEED, qlonglong(0));

    const QModelIndex idx = createIndexForItem(p);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
}

TransferViewItem *TransferViewModel::findUploadRow(const VarMap &params) {
    const QString hub = vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), false, &item, hub) && !hub.isEmpty())
        findTransfer(vstr(params["CID"]), false, &item, QString());
    return item;
}

TransferViewItem *TransferViewModel::uploadScope(TransferViewItem *item) const {
    if (!item || item->download)
        return nullptr;
    if (!item->cid.isEmpty() && item->parent() && item->parent() != rootItem)
        return item->parent();
    return item;
}

bool TransferViewModel::uploadFullyIdle(TransferViewItem *scope) const {
    if (!scope || scope->download)
        return false;

    const qint64 size = scope->data(COLUMN_TRANSFER_SIZE).toLongLong();
    // fpos = sum of completed segment bytes (matches Finished Full), not max offset.
    if (size <= 0 || scope->fpos < size)
        return false;

    if (scope->cid.isEmpty()) {
        if (scope->childCount() < 1)
            return false;
        for (const auto &child : scope->childItems) {
            if (!child->finished && !child->fail)
                return false;
        }
        return true;
    }

    return scope->finished || scope->fail;
}

void TransferViewModel::settleUpload(const VarMap &params, bool segmentDone) {
    updateTransfer(params);
    TransferViewItem *item = findUploadRow(params);
    if (!item)
        return;

    // Segment done; peer connections often stay in STATE_GET until disconnect.
    if (segmentDone)
        item->finished = true;

    TransferViewItem *scope = uploadScope(item);
    if (!scope)
        return;

    scope->fpos += vlng(params.value("SEGP"));
    if (!uploadFullyIdle(scope)) {
        // Same row across consecutive parts; refresh parent after finished is set.
        if (scope->cid.isEmpty()) {
            updateParent(scope);
            const QModelIndex pidx = createIndexForItem(scope);
            if (pidx.isValid()) {
                emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                                 index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
            }
        }
        return;
    }

    // Keep rows for a possible next file on the same connections (no remove flash).
    if (!segmentDone)
        return;

    if (scope->cid.isEmpty()) {
        for (const auto &child : scope->childItems)
            child->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    }
    item->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    item->updateColumn(COLUMN_TRANSFER_STATS, tr("Upload complete"));
    scope->finished = true;
    scope->percent = 100.0;
    scope->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    scope->updateColumn(COLUMN_TRANSFER_STATS, tr("Upload complete"));
    const QModelIndex idx = createIndexForItem(scope);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
}

void TransferViewModel::completeUpload(const VarMap &params){
    if (!params.empty())
        settleUpload(params, true);
}

void TransferViewModel::failUpload(const VarMap &params){
    if (!params.empty())
        settleUpload(params, false);
}

void TransferViewModel::setShowTranferedFilesOnlyState(bool state){
    showTranferedFilesOnly = state;
}

bool TransferViewModel::getShowTranferedFilesOnlyState(){
    return showTranferedFilesOnly;
}

void TransferViewModel::moveTransfer(TransferViewItem *item, TransferViewItem *from, TransferViewItem *to){
    if (!(item && from && to) || !from->childItems.contains(item))
        return;

    beginRemoveRows(createIndexForItem(from), item->row(), item->row());
    {
        from->childItems.removeAt(item->row());
    }
    endRemoveRows();

    beginInsertRows(createIndexForItem(to), to->childCount(), to->childCount());
    {
        to->appendChild(item);
    }
    endInsertRows();
}
