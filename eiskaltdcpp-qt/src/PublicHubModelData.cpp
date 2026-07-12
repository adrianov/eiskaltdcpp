/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubModel.h"
#include "WulforUtil.h"

#include "dcpp/ClientManagerHubGuard.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

QVariant PublicHubModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() > columnCount(QModelIndex()))
        return QVariant();

    PublicHubItem *item = static_cast<PublicHubItem*>(index.internalPointer());

    switch(role) {
        case Qt::DisplayRole:
            if (index.column() == COLUMN_PHUB_SHARED || index.column() == COLUMN_PHUB_MINSHARE)
                return WulforUtil::formatBytes(item->data(index.column()).toULongLong());

            return item->data(index.column());
        case Qt::TextAlignmentRole:
        {
            int i = index.column();

            if (i == COLUMN_PHUB_SHARED || i == COLUMN_PHUB_RATING || i == COLUMN_PHUB_MAXHUBS || i == COLUMN_PHUB_MAXUSERS ||
                i == COLUMN_PHUB_MINSHARE || i == COLUMN_PHUB_MINSLOTS || i == COLUMN_PHUB_REL)
                return Qt::AlignRight;

            break;
        }
        case Qt::BackgroundRole:
        {
            const QString addr = item->data(COLUMN_PHUB_ADDRESS).toString();
            const QString name = item->data(COLUMN_PHUB_NAME).toString();
            if (addr.isEmpty() && name.isEmpty())
                break;
            if (!dcpp::ClientManagerHubGuard::hasActiveHub(_tq(addr), _tq(name), nullptr))
                break;
            // Soft wash so default text stays readable on light and dark themes.
            QColor c = qApp->palette().color(QPalette::Highlight);
            c.setAlpha(56);
            return c;
        }
    }

    return QVariant();
}
