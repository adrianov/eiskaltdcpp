/***************************************************************************
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

QVariant DownloadQueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    DownloadQueueItem *item = static_cast<DownloadQueueItem*>(index.internalPointer());

    switch(role) {
        case Qt::DecorationRole:
        {
            if (item->dir && index.column() == COLUMN_DOWNLOADQUEUE_NAME)
                return WICON_SIZE(WulforUtil::eiFOLDER_BLUE, 16);
            else if (index.column() == COLUMN_DOWNLOADQUEUE_NAME)
                return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_DOWNLOADQUEUE_NAME).toString()), 16);
        }
        case Qt::DisplayRole:
        {
            if ((index.column() == COLUMN_DOWNLOADQUEUE_DOWN || index.column() == COLUMN_DOWNLOADQUEUE_SIZE) && !item->dir)
                return WulforUtil::formatBytes(item->data(index.column()).toLongLong());
            else if ((index.column() == COLUMN_DOWNLOADQUEUE_DOWN || index.column() == COLUMN_DOWNLOADQUEUE_SIZE) && item->dir)
                break;
            else if (index.column() == COLUMN_DOWNLOADQUEUE_PRIO && !item->dir){
                QueueItem::Priority prio = static_cast<QueueItem::Priority>(item->data(COLUMN_DOWNLOADQUEUE_PRIO).toInt());

                QString prio_str = "";

                switch (prio){
                    case QueueItem::PAUSED:
                        prio_str = tr("Paused");
                        break;
                    case QueueItem::LOWEST:
                        prio_str = tr("Lowest");
                        break;
                    case QueueItem::LOW:
                        prio_str = tr("Low");
                        break;
                    case QueueItem::HIGH:
                        prio_str = tr("High");
                        break;
                    case QueueItem::HIGHEST:
                        prio_str = tr("Highest");
                        break;
                    default:
                        prio_str = tr("Normal");
                }

                return prio_str;
            }

            return item->data(index.column());
        }
        case Qt::TextAlignmentRole:
        {
            if (index.column() == COLUMN_DOWNLOADQUEUE_SIZE ||
                index.column() == COLUMN_DOWNLOADQUEUE_ESIZE ||
                index.column() == COLUMN_DOWNLOADQUEUE_DOWN)
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            else
                return static_cast<int>(Qt::AlignLeft);
        }
        case Qt::ForegroundRole:
        {
            QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();

            if (!errors.isEmpty() && errors != tr("No errors"))
                return AppTheme::errorColor();

            break;
        }
        case Qt::BackgroundColorRole:
            break;
        case Qt::ToolTipRole:
        {
            if (item->dir)
                break;

            QString added  = item->data(COLUMN_DOWNLOADQUEUE_ADDED).toString();
            QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();
            QString path   = item->data(COLUMN_DOWNLOADQUEUE_PATH).toString();

            if (errors.isEmpty())
                errors = tr("No errors");

            QString tooltip = QString(tr("<b>Added: </b> %1\n"
                                         "<b>Path: </b> %2\n"
                                         "<b>Errors: </b> %3\n")).arg(added).arg(path).arg(errors);

            return tooltip;
        }
    }

    return QVariant();
}

