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

#include "SearchModel.h"
#include "WulforUtil.h"
#include "AppTheme.h"

namespace {

QVariant nameIcon(SearchItem *item, int column)
{
    if (column != static_cast<int>(COLUMN_SF_FILENAME))
        return QVariant();
    WulforUtil *wu = WulforUtil::getInstance();
    if (item->isDir)
        return WulforUtil::scalePixmap(wu->getPixmapForFolder(), 16);
    return WulforUtil::scalePixmap(
            wu->getPixmapForFile(item->data(COLUMN_SF_FILENAME).toString()), 16);
}

QVariant displayCell(SearchItem *item, int column)
{
    // Non-media / unknown bitrate: blank cell instead of "0".
    if (column == static_cast<int>(COLUMN_SF_BR)
            && item->data(COLUMN_SF_BR).toInt() <= 0)
        return QVariant();
    return item->data(column);
}

QVariant alignCell(int column)
{
    if (column == static_cast<int>(COLUMN_SF_ALLSLOTS)
            || column == static_cast<int>(COLUMN_SF_EXTENSION)
            || column == static_cast<int>(COLUMN_SF_FREESLOTS)
            || column == static_cast<int>(COLUMN_SF_WH))
        return Qt::AlignCenter;
    if (column == static_cast<int>(COLUMN_SF_ESIZE)
            || column == static_cast<int>(COLUMN_SF_SIZE)
            || column == static_cast<int>(COLUMN_SF_COUNT)
            || column == static_cast<int>(COLUMN_SF_BR))
        return Qt::AlignRight;
    return QVariant();
}

QVariant backgroundCell(SearchItem *item)
{
    if (!item->isDir) {
        // Local / queued win over muted gray: stronger signals.
        if (!item->localPath().isEmpty())
            return AppTheme::sharedFileHighlight();
        if (item->isQueued())
            return AppTheme::queuedFileHighlight();
    }
    if (item->mutedTint())
        return AppTheme::offlineSourceHighlight();
    return QVariant();
}

QVariant toolTipCell(SearchItem *item)
{
    if (item->isDir)
        return QVariant();
    const QString path = item->localPath();
    if (!path.isEmpty())
        return SearchModel::tr("File already exists: %1").arg(path);
    if (item->isQueued())
        return SearchModel::tr("Already in download queue");
    return QVariant();
}

} // namespace

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SearchItem *item = static_cast<SearchItem*>(index.internalPointer());
    if (!item)
        return QVariant();

    switch (role) {
        case Qt::DecorationRole:
            return nameIcon(item, index.column());
        case Qt::DisplayRole:
            return displayCell(item, index.column());
        case Qt::TextAlignmentRole:
            return alignCell(index.column());
        case Qt::BackgroundRole:
            return backgroundCell(item);
        case Qt::ToolTipRole:
            return toolTipCell(item);
        default:
            break;
    }

    return QVariant();
}
