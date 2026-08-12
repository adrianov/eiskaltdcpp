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
#include "TransferViewModelTree.h"
#include "transfergrace/TransferViewRemoveUtil.h"
#include "transfersession/TransferSessionRow.h"

namespace {

bool softProgress(const QVariantMap &p, const TransferViewItem *item, const QString &dl,
                  const QString &ul) {
    if (!p.value(QStringLiteral("SOFT_STAT")).toBool() || p.value(QStringLiteral("FAIL")).toBool())
        return false;
    if (p.contains(QStringLiteral("TARGET"))
            && p.value(QStringLiteral("TARGET")).toString() != item->target)
        return false;
    return p.contains(QStringLiteral("STAT"))
        && TransferDisplay::isProgressStat(item->data(COLUMN_TRANSFER_STATS).toString(), dl, ul);
}

bool idleUploadSoft(const QVariantMap &p, const TransferViewItem *item) {
    return !p.value(QStringLiteral("DOWN")).toBool()
        && p.value(QStringLiteral("SOFT_STAT")).toBool()
        && p.value(QStringLiteral("FNAME")).toString().isEmpty()
        && !p.value(QStringLiteral("FAIL")).toBool()
        && !item->finished && !item->fail;
}

} // namespace

TransferViewItem *TransferViewModel::transferForUpdate(const VarMap &params) {
    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)
            && !vbol(params["DOWN"]) && !hub.isEmpty())
        findTransfer(vstr(params["CID"]), false, &item, QString());
    if (item)
        return item;
    if (!vbol(params["DOWN"]) || vbol(params["FAIL"]))
        return nullptr;
    addConnection(params);
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)
            && !hub.isEmpty())
        findTransfer(vstr(params["CID"]), false, &item, QString());
    return item;
}

void TransferViewModel::applyTransferUpdate(TransferViewItem *item, VarMap &p) {
    if (softProgress(p, item, tr("Downloaded "), tr("Uploaded ")))
        p.remove("STAT");

    if (!vbol(p["DOWN"]) && p.contains("SEGP") && !vbol(p["FAIL"]))
        item->segBytes = vlng(p["SEGP"]);

    for (auto i = column_map.constBegin(); i != column_map.constEnd(); ++i) {
        if (p.contains(i.key()))
            item->updateColumn(i.value(), p[i.key()]);
    }

    TransferSessionRow::trackPeer(item, p);

    if (p.contains("TARGET"))
        item->target = vstr(p["TARGET"]);
    item->fail = vbol(p["FAIL"]);
    if (item->fail)
        item->smoothTleft = -1;
    if (!item->fail && item->download) {
        item->finished = false;
        item->finishRank = 0;
    }
    if (p.contains("TTH"))
        item->tth = vstr(p["TTH"]);
}

TransferViewItem *TransferViewModel::parentForUpdate(TransferViewItem *item, const VarMap &p,
                                                    TransferViewItem *from) {
    const QString newTarget = vstr(p["TARGET"]);
    TransferViewItem *existing = nullptr;
    if (from && from != rootItem
            && !findParent(newTarget, &existing, vbol(p["DOWN"]), vstr(p["IP"]))
            && TransferViewTree::retargetGroup(item, from, newTarget, p)) {
        pendingTargetRemoves.remove(from->target);
        grace.cancelDownload(from->target);
        return from;
    }
    return getParent(newTarget, p);
}

void TransferViewModel::placeTransferRow(TransferViewItem *item, const VarMap &p) {
    const QString fname = vstr(p["FNAME"]);
    TransferViewItem *from = item->parent();
    TransferViewItem *to = TransferViewTree::wantsParent(p, fname)
            ? parentForUpdate(item, p, from) : rootItem;

    if (TransferViewTree::isAttached(item) && from == to)
        return;

    if (TransferViewTree::isAttached(item) && from && from != to)
        moveTransfer(item, from, to);
    else if (!(showTranferedFilesOnly && TransferViewTree::isHiddenName(fname)))
        insertUnder(to, item);

    if (from && from != to)
        releaseEmptyGroup(from);
    sort(sortColumn, sortOrder);
}

void TransferViewModel::notifyTransferChange(TransferViewItem *item) {
    if (!item)
        return;
    const QModelIndex idx = createIndexForItem(item);
    if (!idx.isValid())
        return;
    emit dataChanged(index(idx.row(), 0, idx.parent()),
                     index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
}

void TransferViewModel::updateTransfer(const VarMap &params){
    if (params.empty())
        return;

    if (TransferViewRemove::offlineOrphan(vstr(params["CID"]), vstr(params["HOST"]))) {
        removeTransfer(params);
        return;
    }

    TransferViewItem *item = transferForUpdate(params);
    if (!item)
        return;

    if (vbol(params["FAIL"]) && shouldRemoveStaleRow(item)) {
        removeTransfer(params);
        return;
    }

    if (idleUploadSoft(params, item))
        grace.armUpload(params, 10000);

    VarMap p = params;
    applyTransferUpdate(item, p);
    placeTransferRow(item, p);

    if (showTranferedFilesOnly && TransferViewTree::isHiddenName(vstr(p["FNAME"])))
        return;

    TransferSessionRow::setQueuePos(item, rootItem, p);
    TransferSessionRow::publish(item, rootItem, p, tr("Downloaded "), tr("Uploaded "));
    notifyTransferChange(item);

    TransferViewItem *group = item->parent();
    if (group && group != rootItem && rootItem->childItems.contains(group)) {
        updateParent(group);
        notifyTransferChange(group);
    }
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
