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

#include "ProgressBarPaint.h"

#include <QtWidgets>

DownloadQueueDelegate::DownloadQueueDelegate(QObject *parent):
        QStyledItemDelegate(parent)
{
}

DownloadQueueDelegate::~DownloadQueueDelegate(){
}

void DownloadQueueDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    if (index.column() != COLUMN_DOWNLOADQUEUE_STATUS){
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    DownloadQueueItem *item = reinterpret_cast<DownloadQueueItem*>(index.internalPointer());

    if (!item || item->dir) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const qulonglong esize = item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();
    const qulonglong down = item->data(COLUMN_DOWNLOADQUEUE_DOWN).toULongLong();
    const int percent = esize > 0 ? int(down * 100.0 / esize) : 0;
    paintProgressCell(painter, option, percent, index.data(Qt::DisplayRole).toString());
}
