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

void TransferViewModel::updateParents(){
    for (const auto &i : rootItem->childItems) {
        updateParent(i);
        const QModelIndex idx = createIndexForItem(i);
        if (idx.isValid()) {
            emit dataChanged(index(idx.row(), 0, idx.parent()),
                             index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
        }
    }

    // SPEED order changes every tick; other columns stay stable with dataChanged only.
    if (sortColumn == COLUMN_TRANSFER_SPEED)
        sort(sortColumn, sortOrder);
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

        const QModelIndex idx = createIndexForItem(item);
        if (idx.isValid()) {
            emit dataChanged(index(idx.row(), 0, idx.parent()),
                             index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
        }
    }
}
