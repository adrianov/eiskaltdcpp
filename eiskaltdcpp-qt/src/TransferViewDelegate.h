/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QStyledItemDelegate>

#include "TransferViewModel.h"

class TransferViewDelegate:
        public QStyledItemDelegate
{
    Q_OBJECT

public:
    TransferViewDelegate(QObject* = nullptr);
    virtual ~TransferViewDelegate();

    virtual void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private Q_SLOTS:
    void wsVarValueChanged(const QString&, const QVariant &);

private:
    QColor download_bar_color;
    QColor upload_bar_color;
};
