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
#include "TransferDisplay.h"
#include "TransferViewMetrics.h"
#include "TransferViewModelTree.h"
#include "TransferViewRemoveUtil.h"
#include "transfersession/TransferSession.h"

void TransferViewModel::updateTransfer(const VarMap &params){
    if (params.empty())
        return;

    if (TransferViewRemove::offlineOrphan(vstr(params["CID"]), vstr(params["HOST"]))) {
        removeTransfer(params);
        return;
    }

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)
            && !vbol(params["DOWN"]) && !hub.isEmpty())
        findTransfer(vstr(params["CID"]), false, &item, QString());
    if (!item) {
        if (!vbol(params["DOWN"]) || vbol(params["FAIL"]))
            return;
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)
                && !vbol(params["DOWN"]) && !hub.isEmpty())
            findTransfer(vstr(params["CID"]), false, &item, QString());
        if (!item)
            return;
    }

    if (vbol(params["FAIL"]) && shouldRemoveStaleRow(item)) {
        removeTransfer(params);
        return;
    }

    // Soft Connected/Connecting only. Do not re-arm over finished/failed rows —
    // that would reset the grace timer forever while the peer stays parked.
    if (!vbol(params["DOWN"]) && vbol(params.value("SOFT_STAT"))
            && vstr(params.value("FNAME")).isEmpty() && !vbol(params["FAIL"])
            && !item->finished && !item->fail)
        armUploadPrune(params, 10000);

    VarMap p = params;
    // Between segments keep Downloaded/Uploaded; not across a TARGET (next file).
    const bool sameTarget = !p.contains("TARGET") || vstr(p["TARGET"]) == item->target;
    if (vbol(p.value("SOFT_STAT")) && !vbol(p["FAIL"]) && sameTarget) {
        if (p.contains("STAT")
            && TransferDisplay::isProgressStat(item->data(COLUMN_TRANSFER_STATS).toString(),
                                               tr("Downloaded "), tr("Uploaded ")))
            p.remove("STAT");
    }

    if (!vbol(p["DOWN"]) && p.contains("SEGP") && !vbol(p["FAIL"]))
        item->segBytes = vlng(p["SEGP"]);

    for (auto i = column_map.constBegin(); i != column_map.constEnd(); ++i) {
        if (p.contains(i.key()))
            item->updateColumn(i.value(), p[i.key()]);
    }

    // Downloads: DPOS is per-source segment bytes (parent sums them).
    // Uploads: absolute progress comes from TransferSession::writeUi.
    if (vbol(p["DOWN"])) {
        if (p.contains("DPOS"))
            item->dpos = vlng(p["DPOS"]);
        if (p.contains("PERC"))
            item->percent = qBound(0.0, vdbl(p["PERC"]), 100.0);
    }

    if (p.contains("TARGET"))
        item->target = vstr(p["TARGET"]);
    item->fail = vbol(p["FAIL"]);
    // Uploads: segment-complete (finished) is set in completeUpload and cleared in initTransfer.
    if (!item->fail && item->download)
        item->finished = false;
    if (p.contains("TTH"))
        item->tth = vstr(p["TTH"]);
    if (p.contains("QUEUE_POS"))
        item->queuePos = vlng(p["QUEUE_POS"]);

    const QString fname = vstr(p["FNAME"]);
    const QString newTarget = vstr(p["TARGET"]);

    TransferViewItem *from = item->parent();
    TransferViewItem *to = rootItem;
    if (TransferViewTree::wantsParent(p, fname)) {
        TransferViewItem *existing = nullptr;
        const QString oldTarget = from ? from->target : QString();
        const bool isDown = vbol(p["DOWN"]);
        if (from && from != rootItem
                && !findParent(newTarget, &existing, isDown, vstr(p["IP"]))
                && TransferViewTree::retargetGroup(item, from, newTarget, p)) {
            pendingTargetRemoves.remove(oldTarget);
            to = from;
        } else {
            to = getParent(newTarget, p);
        }
    }

    if (!TransferViewTree::isAttached(item) || from != to) {
        if (TransferViewTree::isAttached(item) && from && from != to)
            moveTransfer(item, from, to);
        else if (!(showTranferedFilesOnly && TransferViewTree::isHiddenName(fname)))
            TransferViewTree::attach(item, to);

        if (from && from != rootItem && from != to
                && rootItem->childItems.contains(from)) {
            if (!from->childCount()) {
                beginRemoveRows(QModelIndex(), from->row(), from->row());
                rootItem->childItems.removeAt(from->row());
                delete from;
                endRemoveRows();
            } else {
                // Keep leftover upload hubs for a possible next file (no remove flash).
                updateParent(from);
                const QModelIndex pidx = createIndexForItem(from);
                if (pidx.isValid()) {
                    emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                                     index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
                }
            }
        }
        sort(sortColumn, sortOrder);
    }

    if (showTranferedFilesOnly && TransferViewTree::isHiddenName(fname))
        return;

    // Download parents only — upload parents use fpos for completed segment bytes.
    if (item->download && item->parent() != rootItem
            && rootItem->childItems.contains(item->parent()) && p.contains("FPOS"))
        item->parent()->fpos = vlng(p["FPOS"]);

    // Session rate/progress — skip Connected-only rows (no SEGP yet).
    if (!item->fail) {
        TransferViewItem *scope = TransferSession::scopeOf(item, rootItem);
        TransferSession session(scope);
        if (session.valid() && (scope->speedStart > 0 || p.contains("SEGP"))) {
            if (scope->speedStart == 0) {
                const qlonglong base = item->download
                        ? (p.contains("FPOS") ? vlng(p["FPOS"]) : scope->fpos)
                        : (p.contains("BASE") ? vlng(p["BASE"]) : 0);
                session.begin(base);
            }
            qlonglong fileSize = p.contains("ESIZE")
                    ? vlng(p["ESIZE"]) : vlng(scope->data(COLUMN_TRANSFER_SIZE));
            session.writeUi(p, fileSize);
            item->updateColumn(COLUMN_TRANSFER_SPEED, p["SPEED"]);
            item->updateColumn(COLUMN_TRANSFER_TLEFT, p["TLEFT"]);
            if (!item->download) {
                item->dpos = vlng(p["DPOS"]);
                item->percent = qBound(0.0, vdbl(p["PERC"]), 100.0);
                if (vlng(item->data(COLUMN_TRANSFER_SIZE)) <= 0 && p.contains("ESIZE"))
                    item->updateColumn(COLUMN_TRANSFER_SIZE, p["ESIZE"]);
                const QString stat = item->data(COLUMN_TRANSFER_STATS).toString();
                if (stat.isEmpty()
                        || TransferDisplay::isProgressStat(stat, tr("Downloaded "), tr("Uploaded "))
                        || vbol(p.value("SOFT_STAT"))) {
                    item->updateColumn(COLUMN_TRANSFER_STATS,
                            TransferViewMetrics::uploadProgressStat(
                                    item->dpos, vlng(item->data(COLUMN_TRANSFER_SIZE))));
                }
                if (scope != item) {
                    scope->dpos = item->dpos;
                    scope->percent = item->percent;
                }
            }
        }
    }

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
    TransferViewItem *group = item->parent();
    if (group && group != rootItem && rootItem->childItems.contains(group)) {
        updateParent(group);
        const QModelIndex pidx = createIndexForItem(group);
        if (pidx.isValid()) {
            emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                             index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
        }
    }
}
