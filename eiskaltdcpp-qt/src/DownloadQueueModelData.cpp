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

#include "DownloadQueueModel.h"
#include "WulforUtil.h"
#include "AppTheme.h"

#include <dcpp/stdinc.h>
#include <dcpp/QueueManager.h>

namespace {

bool isBlankDirCol(const DownloadQueueItem *item, int col)
{
    if (!item->dir)
        return false;
    return col == COLUMN_DOWNLOADQUEUE_DOWN
        || col == COLUMN_DOWNLOADQUEUE_SIZE
        || col == COLUMN_DOWNLOADQUEUE_STATUS
        || col == COLUMN_DOWNLOADQUEUE_PRIO;
}

QString statusPercent(const DownloadQueueItem *item)
{
    const qulonglong esize = item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();
    const qulonglong down = item->data(COLUMN_DOWNLOADQUEUE_DOWN).toULongLong();
    const double percent = esize > 0 ? (down * 100.0 / esize) : 0.0;
    return QStringLiteral("%1%").arg(percent, 0, 'f', 1);
}

QString prioLabel(int prio)
{
    switch (static_cast<QueueItem::Priority>(prio)) {
    case QueueItem::PAUSED:  return DownloadQueueModel::tr("Paused");
    case QueueItem::LOWEST:  return DownloadQueueModel::tr("Lowest");
    case QueueItem::LOW:     return DownloadQueueModel::tr("Low");
    case QueueItem::HIGH:    return DownloadQueueModel::tr("High");
    case QueueItem::HIGHEST: return DownloadQueueModel::tr("Highest");
    default:                 return DownloadQueueModel::tr("Normal");
    }
}

QVariant decoration(const DownloadQueueItem *item, int col)
{
    if (col != COLUMN_DOWNLOADQUEUE_NAME)
        return QVariant();
    WulforUtil *wu = WulforUtil::getInstance();
    if (item->dir)
        return WulforUtil::scalePixmap(wu->getPixmapForFolder(), 16);
    return WulforUtil::scalePixmap(
        wu->getPixmapForFile(item->data(COLUMN_DOWNLOADQUEUE_NAME).toString()), 16);
}

QVariant displayText(const DownloadQueueItem *item, int col)
{
    if (isBlankDirCol(item, col))
        return QVariant();
    if (col == COLUMN_DOWNLOADQUEUE_DOWN || col == COLUMN_DOWNLOADQUEUE_SIZE)
        return WulforUtil::formatBytes(item->data(col).toLongLong());
    if (col == COLUMN_DOWNLOADQUEUE_STATUS)
        return statusPercent(item);
    if (col == COLUMN_DOWNLOADQUEUE_PRIO)
        return prioLabel(item->data(COLUMN_DOWNLOADQUEUE_PRIO).toInt());
    return item->data(col);
}

QVariant fileTooltip(const DownloadQueueItem *item)
{
    QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();
    if (errors.isEmpty())
        errors = DownloadQueueModel::tr("No errors");
    return DownloadQueueModel::tr("<b>Added: </b> %1\n"
                                 "<b>Path: </b> %2\n"
                                 "<b>Status: </b> %3\n"
                                 "<b>Errors: </b> %4\n")
            .arg(item->data(COLUMN_DOWNLOADQUEUE_ADDED).toString(),
                 item->data(COLUMN_DOWNLOADQUEUE_PATH).toString(),
                 item->data(COLUMN_DOWNLOADQUEUE_STATUS).toString(),
                 errors);
}

} // namespace

QVariant DownloadQueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const DownloadQueueItem *item =
        static_cast<DownloadQueueItem*>(index.internalPointer());
    const int col = index.column();

    switch (role) {
    case Qt::DecorationRole:
        return decoration(item, col);
    case Qt::DisplayRole:
        return displayText(item, col);
    case Qt::TextAlignmentRole:
        if (col == COLUMN_DOWNLOADQUEUE_SIZE || col == COLUMN_DOWNLOADQUEUE_ESIZE
                || col == COLUMN_DOWNLOADQUEUE_DOWN)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        return static_cast<int>(Qt::AlignLeft);
    case Qt::ForegroundRole: {
        const QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();
        if (!errors.isEmpty() && errors != tr("No errors"))
            return AppTheme::errorColor();
        return QVariant();
    }
    case Qt::ToolTipRole:
        return item->dir ? QVariant() : fileTooltip(item);
    default:
        return QVariant();
    }
}
