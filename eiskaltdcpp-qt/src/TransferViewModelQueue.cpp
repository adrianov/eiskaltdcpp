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

namespace {
constexpr int uploadDonePruneMs = 2000;
constexpr int uploadPartGapPruneMs = 10000;
}

void TransferViewModel::markDownloadComplete(TransferViewItem *item) {
    if (!item)
        return;
    const QString done = tr("Download complete");
    const qlonglong size = item->data(COLUMN_TRANSFER_SIZE).toLongLong();
    item->updateColumn(COLUMN_TRANSFER_STATS, done);
    item->percent = 100.0;
    item->finished = true;
    item->speedStart = 0;
    item->speedBase = 0;
    item->smoothTleft = -1;
    if (size > 0) {
        item->dpos = size;
        item->fpos = size;
    }
    item->updateColumn(COLUMN_TRANSFER_SPEED, qlonglong(0));
    item->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));
}

void TransferViewModel::finishParent(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    TransferViewItem *p = nullptr;
    if (!findParent(vstr(params["TARGET"]), &p))
        return;

    markDownloadComplete(p);
    p->finishRank = ++finishSeq;
    for (TransferViewItem *child : p->childItems)
        markDownloadComplete(child);

    // Newest Download complete first; active rows keep the current column order below.
    sort(sortColumn, sortOrder);
}

TransferViewItem *TransferViewModel::findUploadRow(const VarMap &params) {
    const QString hub = vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), false, &item, hub) && !hub.isEmpty())
        findTransfer(vstr(params["CID"]), false, &item, QString());
    return item;
}

void TransferViewModel::commitUploadSegment(TransferViewItem *item, TransferViewItem *scope,
                                            bool segmentDone) {
    if (segmentDone) {
        item->fail = false;
        if (!item->finished && item->segBytes > 0)
            scope->fpos += item->segBytes;
        item->segBytes = 0;
        item->finished = true; // idle until next Starting; session clock keeps running
        return;
    }
    item->fail = true;
    item->finished = false;
    item->segBytes = 0;
}

void TransferViewModel::showUploadPartial(TransferViewItem *item, TransferViewItem *scope,
                                          const VarMap &params) {
    if (item->fail) {
        item->smoothTleft = -1;
        item->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
        item->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));
    } else {
        // Same session mean vs full file size — no 0 B/s between segments.
        VarMap ui = params;
        TransferSessionRow::publish(item, rootItem, ui, tr("Downloaded "), tr("Uploaded "));
    }
    notifyTransferChange(item);
    if (scope->cid.isEmpty()) {
        updateParent(scope);
        notifyTransferChange(scope);
    }
}

void TransferViewModel::markUploadFinished(TransferViewItem *item, TransferViewItem *scope) {
    if (scope->cid.isEmpty()) {
        for (const auto &child : scope->childItems)
            child->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    }
    const QString done = tr("Upload finished");
    item->smoothTleft = -1;
    item->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    item->updateColumn(COLUMN_TRANSFER_STATS, done);
    scope->finished = true;
    scope->percent = 100.0;
    scope->speedStart = 0;
    scope->speedBase = 0;
    scope->smoothTleft = -1;
    scope->updateColumn(COLUMN_TRANSFER_SPEED, 0.0);
    scope->updateColumn(COLUMN_TRANSFER_TLEFT, qlonglong(-1));
    scope->updateColumn(COLUMN_TRANSFER_STATS, done);
    notifyTransferChange(scope);
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

    commitUploadSegment(item, scope, segmentDone);

    const bool fileDone = segmentDone && vbol(params.value("FILE_DONE"));
    if (fileDone && scope == item) {
        const qlonglong size = scope->data(COLUMN_TRANSFER_SIZE).toLongLong();
        if (size > 0 && scope->fpos < size)
            scope->fpos = size;
    }

    if (!session.uploadDone()) {
        showUploadPartial(item, scope, params);
        // Fail/file: drop soon. Segment gap: wait for next Starting on this connection.
        if (item->fail || fileDone)
            grace.armUpload(params, uploadDonePruneMs);
        else if (segmentDone)
            grace.armUpload(params, uploadPartGapPruneMs);
        return;
    }

    if (segmentDone)
        markUploadFinished(item, scope);
    grace.armUpload(params, uploadDonePruneMs);
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
