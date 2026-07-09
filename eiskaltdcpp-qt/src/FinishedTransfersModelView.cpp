/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersModel.h"
#include "FinishedTransfersModelSort.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"

using namespace dcpp;

int FinishedTransfersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FinishedTransfersItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant FinishedTransfersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FinishedTransfersItem *item = static_cast<FinishedTransfersItem*>(index.internalPointer());

    switch(role) {
        case Qt::DecorationRole:
        {
            if (rootItem == fileItem){
                if (index.column() == COLUMN_FINISHED_NAME)
                    return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_FINISHED_TARGET).toString()), 16);
            }
            break;
        }
        case Qt::DisplayRole:
        {
            if (rootItem == fileItem){
                if (index.column() == COLUMN_FINISHED_ELAPS)
                    return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_ELAPS).toLongLong()/1000L));
                else if (index.column() == COLUMN_FINISHED_SPEED)
                    return tr("%1/s").arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_SPEED).toLongLong()));
                else if (index.column() == COLUMN_FINISHED_TR)
                    return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong());
                else if (index.column() == COLUMN_FINISHED_FULL)
                    return (item->data(COLUMN_FINISHED_FULL).toBool()? "1" : "0");
            }
            else {
                if (index.column() == COLUMN_FINISHED_SPEED)
                    return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_SPEED).toLongLong()/1000L));
                else if (index.column() == COLUMN_FINISHED_TR)
                    return tr("%1/s").arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong()));
                else if (index.column() == COLUMN_FINISHED_USER)
                    return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_USER).toLongLong());
                else if (index.column() == COLUMN_FINISHED_CRC32)
                    return (item->data(COLUMN_FINISHED_CRC32).toBool()? "1" : "0");
            }

            return item->data(index.column());
        }
        default:
            break;
    }

    return QVariant();
}

Qt::ItemFlags FinishedTransfersModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FinishedTransfersModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex FinishedTransfersModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FinishedTransfersItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FinishedTransfersItem*>(parent.internalPointer());

    FinishedTransfersItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex FinishedTransfersModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int FinishedTransfersModel::rowCount(const QModelIndex &parent) const
{
    FinishedTransfersItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FinishedTransfersItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void FinishedTransfersModel::sort(int column, Qt::SortOrder order) {
    emit layoutAboutToBeChanged();

    sortColumn = column;
    sortOrder = order;

    if (rootItem == fileItem)
        FinishedTransfersModelSort::sortFiles(column, order, rootItem->childItems);
    else
        FinishedTransfersModelSort::sortUsers(column, order, rootItem->childItems);

    emit layoutChanged();
}

void FinishedTransfersModel::clearModel(){
    beginResetModel();
    {
        qDeleteAll(userItem->childItems);
        qDeleteAll(fileItem->childItems);

        userItem->childItems.clear();
        fileItem->childItems.clear();

        file_hash.clear();
        user_hash.clear();
    }
    endResetModel();
}
