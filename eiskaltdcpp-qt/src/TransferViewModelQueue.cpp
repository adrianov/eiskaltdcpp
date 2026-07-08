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
#include "WulforUtil.h"

using namespace TransferViewMetrics;

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

void TransferViewModel::updateParents(){
    for (const auto &i : rootItem->childItems)
        updateParent(i);

    emit layoutChanged();
}

void TransferViewModel::updateParent(TransferViewItem *p){
    if (!p || p->childCount() < 1 || p == rootItem)
        return;

    QList<QString> hubs;
    int downloading = 0;
    qint64 bestQueuePos = 0;
    double speed = 0.0;
    qint64 totalSize = 0;
    qlonglong actual = p->dpos;
    qint64 timeLeft = 0;
    double progress = 0.0;

    totalSize = vlng(p->data(COLUMN_TRANSFER_SIZE));

    for (const auto &i : p->childItems){
        if (!i->fail) {
            const double childSpeed = vdbl(i->data(COLUMN_TRANSFER_SPEED));
            speed += childSpeed;
            if (childSpeed > 0)
                downloading++;
            else if (i->queuePos > 0 && (bestQueuePos == 0 || i->queuePos < bestQueuePos))
                bestQueuePos = i->queuePos;
        }

        if (!hubs.contains(vstr(i->data(COLUMN_TRANSFER_HOST))))
            hubs.append(vstr(i->data(COLUMN_TRANSFER_HOST)));

        actual += i->dpos;
    }

    if (actual <= vlng(p->data(COLUMN_TRANSFER_SIZE)))
        p->dpos = actual;

    if (totalSize > 0)
        progress = (double)(p->dpos * 100.0) / totalSize;
    if (speed > 0)
        timeLeft = (totalSize - p->dpos) / speed;

    speed = TransferDisplay::roundSpeed(speed);
    p->smoothTleft = TransferDisplay::smoothTimeLeft(p->smoothTleft, timeLeft);
    timeLeft = p->smoothTleft;

    if (downloading && !p->finished)
        p->updateColumn(COLUMN_TRANSFER_STATS, tr("Downloaded "));
    else if (!p->finished)
        p->updateColumn(COLUMN_TRANSFER_STATS, slotWaitStat(bestQueuePos, true));

    QString stat = vstr(p->data(COLUMN_TRANSFER_STATS)) + WulforUtil::formatDisplayBytes(p->dpos)
                   + QString(" (%1%)").arg(progress, 0, 'f', 1);

    QString hubs_str;
    for (const QString &s : hubs)
        hubs_str += s + " ";

    if (vstr(p->data(COLUMN_TRANSFER_FNAME)).startsWith(QString("TTH: "))){
        QString name = vstr(p->data(COLUMN_TRANSFER_FNAME));
        name.remove(0, QString("TTH: ").length());

        p->updateColumn(COLUMN_TRANSFER_FNAME, name);
    }

    p->updateColumn(COLUMN_TRANSFER_USERS, QString("%1/%2").arg(downloading).arg(p->childCount()));
    p->updateColumn(COLUMN_TRANSFER_FLAGS, "");
    p->updateColumn(COLUMN_TRANSFER_TLEFT, timeLeft);
    p->updateColumn(COLUMN_TRANSFER_HOST, hubs_str);
    p->updateColumn(COLUMN_TRANSFER_SPEED, speed);

    if (!p->finished)
        p->updateColumn(COLUMN_TRANSFER_STATS, stat);

    p->percent = p->percent == 100.0? 100.0 : progress;
}

void TransferViewModel::updateTransferPos(const VarMap &params, qint64 pos){
    if (params.empty() || !params.contains("CID"))
        return;

    TransferViewItem *item;

    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item))
        return;

    if (!item->finished){
        item->dpos = pos;

        emit layoutChanged();
    }
}
