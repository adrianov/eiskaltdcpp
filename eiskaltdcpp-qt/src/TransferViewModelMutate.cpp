/***************************************************************************
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

void TransferViewModel::updateTransfer(const VarMap &params){
    if (params.empty())
        return;

    const QString hub = vbol(params["DOWN"]) ? QString() : vstr(params["HOST"]);
    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub)) {
        // Do not revive after CM::Removed (uploads always; downloads on Failed —
        // e.g. peer close after a finished file list would recreate an empty row).
        if (!vbol(params["DOWN"]) || vbol(params["FAIL"]))
            return;
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item, hub))
            return;
    }

    VarMap p = params;
    // Between segments keep Downloaded/Uploaded; not across a TARGET (next file).
    const bool sameTarget = !p.contains("TARGET") || vstr(p["TARGET"]) == item->target;
    // High-water only for the same peer row + known same file (explicit TARGET match).
    const bool uploadHold = !vbol(p["DOWN"]) && !vbol(p["FAIL"])
            && p.contains("TARGET") && !item->target.isEmpty()
            && vstr(p["TARGET"]) == item->target;
    if (vbol(p.value("SOFT_STAT")) && !vbol(p["FAIL"]) && sameTarget) {
        if (p.contains("STAT")
            && TransferDisplay::isProgressStat(item->data(COLUMN_TRANSFER_STATS).toString(),
                                               tr("Downloaded "), tr("Uploaded ")))
            p.remove("STAT");
    }

    if (p.contains("SPEED"))
        p["SPEED"] = TransferDisplay::roundSpeed(vdbl(p["SPEED"]));
    if (p.contains("TLEFT")) {
        item->smoothTleft = TransferDisplay::smoothTimeLeft(item->smoothTleft, vlng(p["TLEFT"]));
        p["TLEFT"] = item->smoothTleft;
    }

    for (auto i = column_map.constBegin(); i != column_map.constEnd(); ++i) {
        if (p.contains(i.key()))
            item->updateColumn(i.value(), p[i.key()]);
    }

    if (p.contains("DPOS")) {
        const qlonglong next = vlng(p["DPOS"]);
        item->dpos = uploadHold ? TransferDisplay::highWaterBytes(item->dpos, next) : next;
        if (uploadHold) {
            const qlonglong size = vlng(item->data(COLUMN_TRANSFER_SIZE));
            item->percent = size > 0
                ? qBound(0.0, item->dpos * 100.0 / size, 100.0) : 0.0;
            const QString stat = item->data(COLUMN_TRANSFER_STATS).toString();
            if (TransferDisplay::isProgressStat(stat, tr("Downloaded "), tr("Uploaded ")))
                item->updateColumn(COLUMN_TRANSFER_STATS,
                                   TransferViewMetrics::uploadProgressStat(item->dpos, size));
        } else if (p.contains("PERC")) {
            item->percent = qBound(0.0, vdbl(p["PERC"]), 100.0);
        }
    } else if (p.contains("PERC")) {
        item->percent = qBound(0.0, vdbl(p["PERC"]), 100.0);
    }

    if (p.contains("TARGET"))
        item->target = vstr(p["TARGET"]);
    item->fail = vbol(p["FAIL"]);
    if (!item->fail)
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
        if (from && from != rootItem && !findParent(newTarget, &existing, true)
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

        if (from && from != rootItem && from != to && !from->childCount()
                && rootItem->childItems.contains(from)) {
            beginRemoveRows(QModelIndex(), from->row(), from->row());
            rootItem->childItems.removeAt(from->row());
            delete from;
            endRemoveRows();
        }
        sort(sortColumn, sortOrder);
    }

    if (showTranferedFilesOnly && TransferViewTree::isHiddenName(fname))
        return;

    if (item->parent() != rootItem && rootItem->childItems.contains(item->parent()) && p.contains("FPOS"))
        item->parent()->fpos = vlng(p["FPOS"]);

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
