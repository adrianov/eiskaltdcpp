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

#include "FinishedTransfersModel.h"
#include "FinishedTransfersModelSort.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"

using namespace dcpp;

namespace {

QVariant fileIcon(const FinishedTransfersItem *item, int column)
{
    if (column != COLUMN_FINISHED_NAME)
        return QVariant();
    const QString target = item->data(COLUMN_FINISHED_TARGET).toString();
    return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(target), 16);
}

QString flagText(bool on)
{
    return on ? QStringLiteral("1") : QStringLiteral("0");
}

QVariant displayFile(const FinishedTransfersItem *item, int column)
{
    if (column == COLUMN_FINISHED_ELAPS)
        return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_ELAPS).toLongLong() / 1000L));
    if (column == COLUMN_FINISHED_SPEED)
        return FinishedTransfersModel::tr("%1/s")
                .arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_SPEED).toLongLong()));
    if (column == COLUMN_FINISHED_TR)
        return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong());
    if (column == COLUMN_FINISHED_FULL)
        return flagText(item->data(COLUMN_FINISHED_FULL).toBool());
    return item->data(column);
}

QVariant displayUser(const FinishedTransfersItem *item, int column)
{
    if (column == COLUMN_FINISHED_SPEED)
        return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_SPEED).toLongLong() / 1000L));
    if (column == COLUMN_FINISHED_TR)
        return FinishedTransfersModel::tr("%1/s")
                .arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong()));
    if (column == COLUMN_FINISHED_USER)
        return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_USER).toLongLong());
    if (column == COLUMN_FINISHED_CRC32)
        return flagText(item->data(COLUMN_FINISHED_CRC32).toBool());
    return item->data(column);
}

} // namespace

int FinishedTransfersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FinishedTransfersItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant FinishedTransfersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto *item = static_cast<FinishedTransfersItem*>(index.internalPointer());
    const bool byFile = (rootItem == fileItem);

    switch (role) {
    case Qt::DecorationRole:
        return byFile ? fileIcon(item, index.column()) : QVariant();
    case Qt::DisplayRole:
        return byFile ? displayFile(item, index.column()) : displayUser(item, index.column());
    default:
        return QVariant();
    }
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

QModelIndex FinishedTransfersModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FinishedTransfersItem *parentItem = parent.isValid()
            ? static_cast<FinishedTransfersItem*>(parent.internalPointer())
            : rootItem;
    FinishedTransfersItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex FinishedTransfersModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int FinishedTransfersModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    FinishedTransfersItem *parentItem = parent.isValid()
            ? static_cast<FinishedTransfersItem*>(parent.internalPointer())
            : rootItem;
    return parentItem->childCount();
}

void FinishedTransfersModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    sortColumn = column;
    sortOrder = order;
    if (rootItem == fileItem)
        FinishedTransfersModelSort::sortFiles(column, order, rootItem->childItems);
    else
        FinishedTransfersModelSort::sortUsers(column, order, rootItem->childItems);
    emit layoutChanged();
}

void FinishedTransfersModel::clearModel()
{
    beginResetModel();
    qDeleteAll(userItem->childItems);
    qDeleteAll(fileItem->childItems);
    userItem->childItems.clear();
    fileItem->childItems.clear();
    file_hash.clear();
    user_hash.clear();
    endResetModel();
}
