/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

namespace {

void eraseHashEntry(QMultiHash<QString, TransferViewItem*> &hash, TransferViewItem *item) {
    if (!item || item->cid.isEmpty())
        return;

    auto i = hash.find(item->cid);
    while (i != hash.end() && i.key() == item->cid) {
        if (i.value() == item) {
            hash.erase(i);
            return;
        }
        ++i;
    }
}

} // namespace

void TransferViewModel::removeTransfer(const VarMap &params){
    if (params.empty() || vstr(params["CID"]).isEmpty())
        return;

    auto i = transfer_hash.find(vstr(params["CID"]));

    while (i != transfer_hash.end() && i.key() == vstr(params["CID"])){
        if (i.value()->download == vbol(params["DOWN"])){
            TransferViewItem *item = i.value();
            TransferViewItem *p = item->parent();

            beginRemoveRows(createIndexForItem(p), item->row(), item->row());
            {
                p->childItems.removeAt(item->row());
                delete item;
            }
            endRemoveRows();

            transfer_hash.erase(i);

            if (p != rootItem && !p->childCount()){
                beginRemoveRows(QModelIndex(), p->row(), p->row());
                {
                    rootItem->childItems.removeAt(p->row());

                    delete p;
                }
                endRemoveRows();
            }

            return;
        }

        ++i;
    }
}

void TransferViewModel::removeQueueTarget(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    QString target = vstr(params["TARGET"]);
    TransferViewItem *p;

    if (findParent(target, &p)) {
        while (p->childCount() > 0) {
            TransferViewItem *child = p->childItems.last();
            eraseHashEntry(transfer_hash, child);

            const int row = p->childCount() - 1;
            beginRemoveRows(createIndexForItem(p), row, row);
            p->childItems.removeLast();
            delete child;
            endRemoveRows();
        }

        const int parentRow = p->row();
        beginRemoveRows(QModelIndex(), parentRow, parentRow);
        rootItem->childItems.removeAt(parentRow);
        delete p;
        endRemoveRows();
    }

    for (int row = rootItem->childCount() - 1; row >= 0; --row) {
        TransferViewItem *item = rootItem->child(row);
        if (!item->download || item->target != target)
            continue;

        eraseHashEntry(transfer_hash, item);
        beginRemoveRows(QModelIndex(), row, row);
        rootItem->childItems.removeAt(row);
        delete item;
        endRemoveRows();
    }
}

void TransferViewModel::finishParent(const VarMap &params){
    if (params.empty() || !params.contains("TARGET"))
        return;

    QString target = vstr(params["TARGET"]);
    TransferViewItem *p;

    if (!findParent(target, &p))
        return;

    p->updateColumn(COLUMN_TRANSFER_STATS, tr("Finished"));
    p->percent = 100.0;
    p->finished = true;
    p->updateColumn(COLUMN_TRANSFER_SPEED, qlonglong(0));

    for (const auto &i : p->childItems){
        i->updateColumn(COLUMN_TRANSFER_STATS, tr("Finished"));
        i->percent = 100.0;
        i->finished = true;
        i->updateColumn(COLUMN_TRANSFER_SPEED, qlonglong(0));
    }

    emit layoutChanged();
}
