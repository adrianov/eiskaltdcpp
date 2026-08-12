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
#include "transfergrace/TransferViewRemoveUtil.h"
#include "transfersession/TransferSession.h"
#include "transfersession/TransferSessionRow.h"

namespace {

/** Clear complete state before applying metrics for the next upload file/parts. */
void prepareUploadReuse(TransferViewItem *item, TransferViewItem *root, const QVariantMap &params)
{
    item->finished = false;
    item->segBytes = 0;

    TransferViewItem *scope = TransferSession::scopeOf(item, root);
    if (!scope)
        return;

    const QString newTarget = params.value(QStringLiteral("TARGET")).toString();
    const bool nextFile = (scope == item && TransferViewTree::targetChanged(item, newTarget))
            || TransferViewTree::scopeFullyDone(scope);
    if (nextFile) {
        TransferViewTree::clearByteProgress(scope);
        scope->percent = 0.0;
    }
    scope->finished = false;
    // Once per file — keeps the mean across segments (BASE ignored if already begun).
    TransferSession(scope).begin(params.value(QStringLiteral("BASE")).toLongLong());
}

/** Same peer, next download segment: commit in-flight bytes or reset on retarget. */
void prepareDownloadReuse(TransferViewItem *item, const QVariantMap &params)
{
    item->finished = false;
    if (TransferViewTree::targetChanged(item, params.value(QStringLiteral("TARGET")).toString()))
        TransferViewTree::clearByteProgress(item);
    else
        TransferSessionRow::commitSegment(item);
}

} // namespace

void TransferViewModel::initTransfer(const VarMap &params)
{
    if (params.empty())
        return;

    const bool download = vbol(params["DOWN"]);
    const QString hub = download ? QString() : vstr(params["HOST"]);
    if (!download)
        grace.cancelUpload(vstr(params["CID"]), hub);

    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), download, &item, hub)) {
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), download, &item, hub))
            return;
    }

    if (!item->download)
        prepareUploadReuse(item, rootItem, params);
    else
        prepareDownloadReuse(item, params);

    updateTransfer(params);
}

void TransferViewModel::addConnection(const VarMap &params)
{
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

    insertUnder(parent, item);

    // Added alone never reaches updateTransfer — still drop if Starting never comes.
    if (!bDownload && fname.isEmpty())
        grace.armUpload(params, 10000);
}
