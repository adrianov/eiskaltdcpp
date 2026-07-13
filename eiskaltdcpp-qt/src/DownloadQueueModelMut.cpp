/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "DownloadQueueModel.h"
#include "DownloadQueueModelPrivate.h"
#include "WulforUtil.h"

#include <QApplication>

DownloadQueueItem *DownloadQueueModel::addItem(const QVariantMap &map){
    static quint64 counter = 0;

    DownloadQueueItem *droot = createPath(map["PATH"].toString());

    if (!droot)
        return nullptr;

    DownloadQueueItem *child = nullptr;
    QList<QVariant> childData;

    childData << map["FNAME"]
              << map["STATUS"]
              << (map["ESIZE"].toLongLong() > 0? map["ESIZE"] : 0)
              << (map["DOWN"].toLongLong() > 0? map["DOWN"] : 0)
              << map["PRIO"]
              << map["USERS"]
              << map["PATH"]
              << (map["ESIZE"].toLongLong() > 0? map["ESIZE"] : 0)
              << map["ERRORS"]
              << map["ADDED"]
              << map["TTH"];

    child = new DownloadQueueItem(childData, droot);

    const QModelIndex parentIdx = createIndexForItem(droot);
    const int row = droot->childCount();
    beginInsertRows(parentIdx, row, row);
    droot->appendChild(child);
    endInsertRows();

    Q_D(static DownloadQueueModel);

    d->total_files++;
    d->total_size += childData.at(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    emit updateStats(d->total_files, d->total_size);
    // Expand leaf folder and ancestors so a new row is visible if parents were collapsed.
    for (QModelIndex idx = parentIdx; idx.isValid(); idx = idx.parent())
        emit needExpand(idx);

    counter++;

    if ((counter % 100) == 0)
        QApplication::processEvents();

    return child;
}

void DownloadQueueModel::updItem(const QVariantMap &map){
    DownloadQueueItem *item = createPath(map["PATH"].toString());
    Q_D(static DownloadQueueModel);

    QString target_name = map["FNAME"].toString();
    DownloadQueueItem *target = findTarget(item, target_name);

    if (!target && !(target = addItem(map)))
        return;

    item = target;

    d->total_size -= item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    item->updateColumn(COLUMN_DOWNLOADQUEUE_STATUS, map["STATUS"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_DOWN, (map["DOWN"].toLongLong() > 0? map["DOWN"] : 0));
    item->updateColumn(COLUMN_DOWNLOADQUEUE_ESIZE, map["ESIZE"].toULongLong() > 0? map["ESIZE"] : 0);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_SIZE, map["ESIZE"].toULongLong() > 0? map["ESIZE"] : 0);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_PRIO, map["PRIO"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_USER, map["USERS"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_ERR, map["ERRORS"]);

    d->total_size += item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    emit updateStats(d->total_files, d->total_size);

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid())
        emit dataChanged(idx, index(idx.row(), columnCount() - 1, idx.parent()));
}

