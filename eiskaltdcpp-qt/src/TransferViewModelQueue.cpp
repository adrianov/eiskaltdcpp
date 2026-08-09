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
#include "transfersession/TransferSession.h"
#include "transfersession/TransferSessionRow.h"

void TransferViewModel::finishParent(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    TransferViewItem *p = nullptr;
    if (!findParent(vstr(params["TARGET"]), &p))
        return;

    p->updateColumn(COLUMN_TRANSFER_STATS, tr("Finished"));
    p->percent = 100.0;
    p->finished = true;
    p->speedStart = 0;
    p->speedBase = 0;
    p->smoothTleft = -1;
    p->updateColumn(COLUMN_TRANSFER_SPEED, qlonglong(0));
    p->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));

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

void TransferViewModel::settleUpload(const VarMap &params, bool segmentDone) {
    updateTransfer(params);
    TransferViewItem *item = findUploadRow(params);
    if (!item)
        return;

    TransferSession session(TransferSession::scopeOf(item, rootItem));
    if (!session.valid())
        return;
    TransferViewItem *scope = session.item();

    // Commit this segment into the file session once (segBytes from updateTransfer).
    if (segmentDone) {
        item->fail = false;
        if (!item->finished && item->segBytes > 0)
            scope->fpos += item->segBytes;
        item->segBytes = 0;
        item->finished = true; // idle until next Starting; session clock keeps running
    } else {
        item->fail = true;
        item->finished = false;
        item->segBytes = 0;
    }

    const bool fileDone = segmentDone && vbol(params.value("FILE_DONE"));
    if (fileDone && scope == item) {
        const qlonglong size = scope->data(COLUMN_TRANSFER_SIZE).toLongLong();
        if (size > 0 && scope->fpos < size)
            scope->fpos = size;
    }

    // Fail/file: drop soon. Segment gap: wait for next Starting on this connection.
    static const int donePruneMs = 2000;
    static const int partGapPruneMs = 10000;

    auto notify = [this](TransferViewItem *row) {
        const QModelIndex idx = createIndexForItem(row);
        if (idx.isValid()) {
            emit dataChanged(index(idx.row(), 0, idx.parent()),
                             index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
        }
    };

    if (!session.uploadDone()) {
        if (item->fail) {
            item->smoothTleft = -1;
            item->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
            item->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));
        } else {
            // Same session mean vs full file size — no 0 B/s between segments.
            VarMap ui = params;
            TransferSessionRow::publish(item, rootItem, ui, tr("Downloaded "), tr("Uploaded "));
        }
        notify(item);
        if (scope->cid.isEmpty()) {
            updateParent(scope);
            notify(scope);
        }
        if (item->fail || fileDone)
            armUploadPrune(params, donePruneMs);
        else if (segmentDone)
            armUploadPrune(params, partGapPruneMs);
        return;
    }

    if (segmentDone) {
        if (scope->cid.isEmpty()) {
            for (const auto &child : scope->childItems)
                child->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
        }
        item->smoothTleft = -1;
        item->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
        item->updateColumn(COLUMN_TRANSFER_STATS, tr("Upload finished"));
        scope->finished = true;
        scope->percent = 100.0;
        scope->speedStart = 0;
        scope->speedBase = 0;
        scope->smoothTleft = -1;
        scope->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
        scope->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));
        scope->updateColumn(COLUMN_TRANSFER_STATS, tr("Upload finished"));
        notify(scope);
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
    from->childItems.removeAt(item->row());
    endRemoveRows();

    beginInsertRows(createIndexForItem(to), to->childCount(), to->childCount());
    to->appendChild(item);
    endInsertRows();
}
