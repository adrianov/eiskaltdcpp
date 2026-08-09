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
#include "TransferViewModelTree.h"
#include "TransferViewRemoveUtil.h"
#include "transfersession/TransferSession.h"
#include "transfersession/TransferSessionRow.h"

void TransferViewModel::initTransfer(const VarMap &params){
    if (params.empty())
        return;

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    if (!vbol(params["DOWN"]))
        cancelUploadPrune(vstr(params["CID"]), hub);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)) {
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub))
            return;
    }

    // Reuse the row for the next file/parts — clear complete state before applying metrics.
    if (item && !item->download) {
        TransferViewItem *scope = TransferSession::scopeOf(item, rootItem);
        const qlonglong size = scope
                ? scope->data(COLUMN_TRANSFER_SIZE).toLongLong() : 0;
        const QString newTarget = vstr(params.value("TARGET"));
        const bool leafRetarget = scope == item && !item->target.isEmpty()
                && !newTarget.isEmpty() && newTarget != item->target;
        const bool fullyDone = scope && scope->finished
                && (scope->percent >= 100.0 || (size > 0 && scope->fpos >= size));
        const bool nextFile = leafRetarget || fullyDone;
        item->finished = false;
        item->segBytes = 0;
        if (scope) {
            if (nextFile) {
                scope->fpos = 0;
                scope->dpos = 0;
                scope->percent = 0.0;
                scope->smoothTleft = -1;
                scope->speedStart = 0;
                scope->speedBase = 0;
            }
            scope->finished = false;
            // Once per file — keeps the mean across segments (BASE ignored if already begun).
            TransferSession(scope).begin(vlng(params.value("BASE")));
        }
    } else if (item) {
        // Same peer, next segment: commit in-flight bytes; keep mean rate + peer total.
        item->finished = false;
        const QString newTarget = vstr(params.value("TARGET"));
        if (!item->target.isEmpty() && !newTarget.isEmpty() && newTarget != item->target) {
            item->fpos = 0;
            item->dpos = 0;
            item->segBytes = 0;
            item->speedStart = 0;
            item->speedBase = 0;
        } else {
            TransferSessionRow::commitSegment(item);
        }
    }

    updateTransfer(params);
}

void TransferViewModel::addConnection(const VarMap &params){
    if (params.empty())
        return;

    if (TransferViewRemove::offlineOrphan(vstr(params["CID"]), vstr(params["HOST"])))
        return;

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *existing = nullptr;
    if (findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &existing, hub))
        return;

    const bool bDownload = vbol(params["DOWN"]);
    const QString fname = vstr(params["FNAME"]);
    // Defer tree insert for empty/hidden names when filtering; attach once FNAME is known.
    // Do not create a file parent yet — that would leave an empty orphan row.
    const bool deferAttach = showTranferedFilesOnly && TransferViewTree::isHiddenName(fname);
    const bool bGroup = !deferAttach && TransferViewTree::wantsParent(params, fname);
    TransferViewItem *to = bGroup ? getParent(vstr(params["TARGET"]), params) : nullptr;
    TransferViewItem *parent = (to && bGroup) ? to : rootItem;

    QList<QVariant> data;
    data << params["USER"] << "" << params["STAT"] << "" << "" << ""
         << params["FNAME"] << params["HOST"] << params["TAG"]
         << params.value("IP") << "";

    TransferViewItem *item = new TransferViewItem(data, parent);
    item->download = bDownload;
    item->cid = vstr(params["CID"]);
    item->target = vstr(params["TARGET"]);

    transfer_hash.insert(item->cid, item);

    if (deferAttach)
        return;

    TransferViewTree::attach(item, parent);
    emit layoutChanged();

    // Added alone never reaches updateTransfer — still drop if Starting never comes.
    if (!bDownload && fname.isEmpty())
        armUploadPrune(params, 10000);
}
