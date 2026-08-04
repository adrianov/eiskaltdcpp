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

    TransferViewItem *scope = uploadScope(item);
    if (!scope)
        return;

    // SEGP is per-Upload getPos() (one Complete per Upload). Skip fails and
    // double-settles so a retry cannot push fpos past the real total early.
    const bool alreadySettled = item->finished;
    if (segmentDone && !alreadySettled) {
        item->finished = true;
        item->fail = false;
        const qlonglong seg = vlng(params.value("SEGP"));
        if (seg > 0)
            scope->fpos += seg;
    } else if (!segmentDone) {
        item->fail = true;
        item->finished = false;
    }

    // Last segment of a leaf row: align fpos if earlier parts under-counted.
    // Do not snap a group parent — siblings may still be uploading.
    const bool fileDone = segmentDone && vbol(params.value("FILE_DONE"));
    if (fileDone && scope == item) {
        const qlonglong size = scope->data(COLUMN_TRANSFER_SIZE).toLongLong();
        if (size > 0 && scope->fpos < size)
            scope->fpos = size;
    }

    // Brief flash for complete/fail; Starting cancels if the next file arrives.
    static const int donePruneMs = 2000;

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
        // Failures, or fileDone while siblings still busy — not mid-file parts.
        if (item->fail || fileDone)
            armUploadPrune(params, donePruneMs);
        return;
    }

    // Failures keep their error text; completed files show Upload complete.
    // Either way, drop after grace if the peer stays idle (no next Starting).
    if (segmentDone) {
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
    armUploadPrune(params, donePruneMs);
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
