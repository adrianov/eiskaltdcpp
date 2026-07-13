/***************************************************************************
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

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SearchItem *item = static_cast<SearchItem*>(index.internalPointer());
    if (!item)
        return QVariant();

    switch(role) {
        case Qt::DecorationRole:
        {
            if (index.column() == COLUMN_SF_FILENAME && !item->isDir)
                return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_SF_FILENAME).toString()), 16);
            else if (index.column() == COLUMN_SF_FILENAME && item->isDir)
                return WICON_SIZE(WulforUtil::eiFOLDER_BLUE, 16);
            break;
        }
        case Qt::DisplayRole:
            return item->data(index.column());
        case Qt::TextAlignmentRole:
        {
            const int i_column = index.column();
            bool align_center = (i_column == COLUMN_SF_ALLSLOTS) || (i_column == COLUMN_SF_EXTENSION) ||
                                (i_column == COLUMN_SF_FREESLOTS);
            bool align_right  = (i_column == COLUMN_SF_ESIZE) || (i_column == COLUMN_SF_SIZE ) || (i_column == COLUMN_SF_COUNT);

            if (align_center)
                return Qt::AlignCenter;
            else if (align_right)
                return Qt::AlignRight;

            break;
        }
        case Qt::BackgroundRole:
        {
            if (item->isDir)
                break;
            // Local file wins over queued: already-have is the stronger signal.
            if (!item->localPath().isEmpty())
                return AppTheme::sharedFileHighlight();
            if (item->isQueued())
                return AppTheme::queuedFileHighlight();
            break;
        }
        case Qt::ToolTipRole:
        {
            const QString path = item->localPath();
            if (!path.isEmpty())
                return tr("File already exists: %1").arg(path);
            if (item->isQueued())
                return tr("Already in download queue");
            break;
        }
        default: break;
    }

    return QVariant();
}
