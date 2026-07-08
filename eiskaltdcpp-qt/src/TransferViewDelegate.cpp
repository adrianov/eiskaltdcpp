/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewDelegate.h"

#include "ProgressBarPaint.h"
#include "WulforSettings.h"

TransferViewDelegate::TransferViewDelegate(QObject *parent):
        QStyledItemDelegate(parent)
{
    download_bar_color = qvariant_cast<QColor>(WVGET("transferview/download-bar-color", QColor()));
    upload_bar_color = qvariant_cast<QColor>(WVGET("transferview/upload-bar-color", QColor()));

    connect(WulforSettings::getInstance(), SIGNAL(varValueChanged(QString,QVariant)), this, SLOT(wsVarValueChanged(QString,QVariant)));
}

TransferViewDelegate::~TransferViewDelegate(){
}

void TransferViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    TransferViewItem *item = reinterpret_cast<TransferViewItem*>(index.internalPointer());

    if (index.column() != COLUMN_TRANSFER_STATS || !item) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    if (item->download && item->childCount() == 1)
        item = item->child(0);

#if defined(USE_PROGRESS_BARS)
    QPalette pal = option.palette;
    if (item->download && download_bar_color.isValid())
        pal.setColor(QPalette::Highlight, download_bar_color);
    else if (!item->download && upload_bar_color.isValid())
        pal.setColor(QPalette::Highlight, upload_bar_color);

    paintProgressCell(painter, option, static_cast<int>(item->percent),
                      item->data(COLUMN_TRANSFER_STATS).toString(), &pal);
#else
    paintProgressCell(painter, option, static_cast<int>(item->percent),
                      item->data(COLUMN_TRANSFER_STATS).toString());
#endif
}

void TransferViewDelegate::wsVarValueChanged(const QString &key, const QVariant &val){
    if (key == "transferview/download-bar-color")
        download_bar_color = qvariant_cast<QColor>(val);
    else if (key == "transferview/upload-bar-color")
        upload_bar_color = qvariant_cast<QColor>(val);
}
