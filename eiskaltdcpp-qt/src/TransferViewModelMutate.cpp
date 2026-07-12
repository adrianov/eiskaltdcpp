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

namespace {

bool isHiddenTransferName(const QString &fname)
{
    return fname.isEmpty() || fname == TransferViewModel::tr("File list");
}

bool isAttached(TransferViewItem *item)
{
    return item && item->parent() && item->parent()->childItems.contains(item);
}

void attachTransfer(TransferViewItem *item, TransferViewItem *parent)
{
    if (!item || !parent || isAttached(item))
        return;
    parent->appendChild(item);
}

} // namespace

void TransferViewModel::initTransfer(const VarMap &params){
    if (params.empty())
        return;

    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item)) {
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item))
            return;
    }

    const bool needParent = (vstr(params["FNAME"]) != tr("File list"));
    if (needParent){
        TransferViewItem *to = getParent(vstr(params["TARGET"]), params);
        TransferViewItem *p = item->parent();

        if (p != to) {
            if (isAttached(item))
                moveTransfer(item, p, to);
            else
                attachTransfer(item, to);

            if (p && p != rootItem && !p->childCount() && rootItem->childItems.contains(p)){
                beginRemoveRows(QModelIndex(), p->row(), p->row());
                rootItem->childItems.removeAt(p->row());
                delete p;
                endRemoveRows();
            }
            sort(sortColumn, sortOrder);
        } else if (!isAttached(item)) {
            attachTransfer(item, to);
            emit layoutChanged();
        }
    } else if (!isAttached(item)) {
        attachTransfer(item, rootItem);
        emit layoutChanged();
    }

    updateTransfer(params);
}

void TransferViewModel::addConnection(const VarMap &params){
    if (params.empty())
        return;

    TransferViewItem *existing = nullptr;
    if (findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &existing))
        return;

    const bool bDownload = vbol(params["DOWN"]);
    const bool bGroup = bDownload && vbol(params["BGROUP"]);
    TransferViewItem *to = bGroup ? getParent(vstr(params["TARGET"]), params) : nullptr;
    TransferViewItem *parent = (to && bGroup) ? to : rootItem;

    QList<QVariant> data;
    data << params["USER"] << "" << params["STAT"] << "" << "" << ""
         << params["FNAME"] << params["HOST"] << "" << "";

    TransferViewItem *item = new TransferViewItem(data, parent);
    item->download = bDownload;
    item->cid = vstr(params["CID"]);
    if (item->download && bGroup)
        item->target = vstr(params["TARGET"]);

    transfer_hash.insert(item->cid, item);

    // Defer tree insert for empty names when filtering; attach once FNAME is known.
    if (showTranferedFilesOnly && isHiddenTransferName(vstr(params["FNAME"])))
        return;

    attachTransfer(item, parent);
    emit layoutChanged();
}

void TransferViewModel::updateTransfer(const VarMap &params){
    if (params.empty())
        return;

    TransferViewItem *item = nullptr;
    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item)) {
        // Uploads come from CM::Added only; do not revive after CM::Removed.
        if (!vbol(params["DOWN"]))
            return;
        addConnection(params);
        if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item))
            return;
    }

    VarMap p = params;
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

    item->dpos = vlng(p["DPOS"]);
    if (vbol(p["DOWN"]) || p.contains("PERC"))
        item->percent = vdbl(p["PERC"]);

    item->target = vstr(p["TARGET"]);
    item->fail = vbol(p["FAIL"]);
    item->tth = vstr(p["TTH"]);
    if (p.contains("QUEUE_POS"))
        item->queuePos = vlng(p["QUEUE_POS"]);

    const QString fname = vstr(p["FNAME"]);
    if (showTranferedFilesOnly && isHiddenTransferName(fname))
        return;

    if (!isAttached(item)) {
        TransferViewItem *parent = item->parent() ? item->parent() : rootItem;
        if (vbol(p["DOWN"]) && !vstr(p["TARGET"]).isEmpty() && fname != tr("File list"))
            parent = getParent(vstr(p["TARGET"]), p);
        attachTransfer(item, parent);
        emit layoutChanged();
    }

    if (item->parent() != rootItem && rootItem->childItems.contains(item->parent()) && p.contains("FPOS"))
        item->parent()->dpos = vlng(p["FPOS"]);

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
}
