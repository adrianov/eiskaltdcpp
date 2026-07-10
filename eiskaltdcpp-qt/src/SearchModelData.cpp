/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"
#include "SearchLocalPath.h"
#include "WulforUtil.h"
#include "AppTheme.h"

using namespace dcpp;

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SearchItem *item = static_cast<SearchItem*>(index.internalPointer());

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
        case Qt::ForegroundRole:
        {
            if (item->isDir)
                break;

            const QString localPath = SearchLocalPath::resolve(item->data(COLUMN_SF_TTH).toString());
            if (!localPath.isEmpty()) {
                static QColor c;
                c.setNamedColor(AppTheme::chatColor(WS_APP_SHARED_FILES_COLOR));
                c.setAlpha(WIGET(WI_APP_SHARED_FILES_ALPHA));
                return c;
            }
            break;
        }
        case Qt::BackgroundColorRole:
            break;
        case Qt::ToolTipRole:
        {
            if (item->isDir)
                break;

            const QString localPath = SearchLocalPath::resolve(item->data(COLUMN_SF_TTH).toString());
            if (!localPath.isEmpty())
                return tr("File already exists: %1").arg(localPath);
            break;
        }
        default: break;
    }

    return QVariant();
}
