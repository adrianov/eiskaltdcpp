/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "DownloadQueueModel.h"

#include "ProgressBarPaint.h"
#include "WulforUtil.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

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

    const qulonglong esize = item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toLongLong();
    double percent = ((double)item->data(COLUMN_DOWNLOADQUEUE_DOWN).toLongLong() * 100.0);
    percent = (esize > 0) ? (percent/(double)esize) : 0.0;
    const QString status = QString("%1%").arg(percent, 0, 'f', 1);

    paintProgressCell(painter, option, static_cast<int>(percent), status);
}
