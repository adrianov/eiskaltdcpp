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
#include "WulforUtil.h"
#include "transfersession/TransferSession.h"

using namespace TransferViewMetrics;

void TransferViewModel::updateParents(){
    for (const auto &i : rootItem->childItems) {
        if (i->childCount() < 1)
            continue;
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

    pruneEmptyParents();
}

void TransferViewModel::updateParent(TransferViewItem *p){
    if (!p || p->childCount() < 1 || p == rootItem)
        return;

    QList<QString> hubs;
    QList<QString> tags;
    int active = 0;
    qint64 bestQueuePos = 0;
    qint64 totalSize = vlng(p->data(COLUMN_TRANSFER_SIZE));
    // Downloads: committed fpos + in-flight segment bytes.
    // Uploads: absolute session progress (speedBase + Σ part bytes), not file-offset high-water.
    qlonglong progressPos = 0;

    for (const auto &i : p->childItems){
        if (!i->fail) {
            // Downloads: active = sources with bytes in flight (file speed is shared).
            if (p->download) {
                if (i->dpos > 0)
                    active++;
                else if (i->queuePos > 0 && (bestQueuePos == 0 || i->queuePos < bestQueuePos))
                    bestQueuePos = i->queuePos;
            } else {
                const double childSpeed = i->finished
                        ? 0.0 : vdbl(i->data(COLUMN_TRANSFER_SPEED));
                if (childSpeed > 0)
                    active++;
                else if (i->queuePos > 0 && (bestQueuePos == 0 || i->queuePos < bestQueuePos))
                    bestQueuePos = i->queuePos;
            }
        }

        if (!hubs.contains(vstr(i->data(COLUMN_TRANSFER_HOST))))
            hubs.append(vstr(i->data(COLUMN_TRANSFER_HOST)));

        const QString tag = vstr(i->data(COLUMN_TRANSFER_TAG));
        if (!tag.isEmpty() && !tags.contains(tag))
            tags.append(tag);

        const qint64 childSize = vlng(i->data(COLUMN_TRANSFER_SIZE));
        if (childSize > totalSize)
            totalSize = childSize;

        if (p->download)
            progressPos += i->dpos;
    }

    if (p->download) {
        progressPos += p->fpos;
        if (totalSize > 0 && progressPos > totalSize)
            progressPos = totalSize;
        if (!p->finished)
            progressPos = TransferDisplay::highWaterBytes(p->dpos, progressPos);
    } else {
        progressPos = TransferSession(p).progressBytes();
        if (totalSize > 0 && progressPos > totalSize)
            progressPos = totalSize;
    }
    p->dpos = progressPos;

    if (totalSize > 0)
        p->updateColumn(COLUMN_TRANSFER_SIZE, totalSize);

    const double progress = totalSize > 0
        ? qBound(0.0, (double)(p->dpos * 100.0) / totalSize, 100.0) : 0.0;
    p->percent = progress;

    VarMap speedParams;
    TransferSession(p).writeUi(speedParams, totalSize);
    double speed = vdbl(speedParams["SPEED"]);
    qint64 timeLeft = vlng(speedParams["TLEFT"]);
    if (timeLeft < 0)
        timeLeft = 0;
    // Settled partial uploads still have session bytes; do not show a decaying ghost rate.
    if (!p->download && active == 0) {
        speed = 0.0;
        timeLeft = 0;
        p->speedStart = 0;
    }

    // Keep progress wording while sources briefly idle between segments (speed→0).
    if (!p->finished && (active || p->dpos > 0 || p->percent > 0))
        p->updateColumn(COLUMN_TRANSFER_STATS, p->download ? tr("Downloaded ") : tr("Uploaded "));
    else if (!p->finished && p->download)
        p->updateColumn(COLUMN_TRANSFER_STATS, slotWaitStat(bestQueuePos, true));
    else if (!p->finished)
        p->updateColumn(COLUMN_TRANSFER_STATS, tr("Uploaded "));

    const QString stat = vstr(p->data(COLUMN_TRANSFER_STATS)) + WulforUtil::formatDisplayBytes(p->dpos)
                   + QString(" (%1%)").arg(progress, 0, 'f', 1);

    QString hubs_str;
    for (const QString &s : hubs)
        hubs_str += s + " ";

    QString tags_str;
    for (const QString &s : tags)
        tags_str += s + " ";

    if (vstr(p->data(COLUMN_TRANSFER_FNAME)).startsWith(QString("TTH: "))){
        QString name = vstr(p->data(COLUMN_TRANSFER_FNAME));
        name.remove(0, QString("TTH: ").length());
        p->updateColumn(COLUMN_TRANSFER_FNAME, name);
    }

    QString nick;
    for (const auto &i : p->childItems) {
        nick = vstr(i->data(COLUMN_TRANSFER_USERS));
        if (!nick.isEmpty())
            break;
    }
    p->updateColumn(COLUMN_TRANSFER_USERS,
                    p->childCount() > 1 && !nick.isEmpty()
                        ? nick + QString(", ... (%1/%2)").arg(active).arg(p->childCount())
                        : nick);
    p->updateColumn(COLUMN_TRANSFER_FLAGS, "");
    p->updateColumn(COLUMN_TRANSFER_TLEFT, timeLeft);
    p->updateColumn(COLUMN_TRANSFER_HOST, hubs_str);
    p->updateColumn(COLUMN_TRANSFER_TAG, tags_str);
    p->updateColumn(COLUMN_TRANSFER_SPEED, speed);

    if (!p->finished)
        p->updateColumn(COLUMN_TRANSFER_STATS, stat);
}

void TransferViewModel::updateTransferPos(const VarMap &params, qint64 pos){
    if (params.empty() || !params.contains("CID"))
        return;

    TransferViewItem *item;

    if (!findTransfer(vstr(params["CID"]), vbol(params["DOWN"]), &item,
                      vbol(params["DOWN"]) ? QString() : vstr(params["HOST"])))
        return;

    if (item->finished)
        return;

    // Downloads: file-level position belongs on the parent (GTK-style), not as child
    // segment dpos — absolute child dpos made parent FPOS+Σdpos jump on every tick.
    if (vbol(params["DOWN"])) {
        TransferViewItem *group = item->parent();
        if (group && group != rootItem && rootItem->childItems.contains(group)) {
            group->dpos = TransferDisplay::highWaterBytes(group->dpos, pos);
            updateParent(group);
            const QModelIndex pidx = createIndexForItem(group);
            if (pidx.isValid()) {
                emit dataChanged(index(pidx.row(), 0, pidx.parent()),
                                 index(pidx.row(), columnCount(pidx.parent()) - 1, pidx.parent()));
            }
        }
        return;
    }

    // Upload progress is updated via Tick/Starting (updateTransfer), not this signal.
    item->dpos = pos;

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid()) {
        emit dataChanged(index(idx.row(), 0, idx.parent()),
                         index(idx.row(), columnCount(idx.parent()) - 1, idx.parent()));
    }
}
