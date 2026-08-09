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

#include "TransferViewModel.h"
#include "WulforUtil.h"

#include <QTime>

namespace {

QVariant transferIcon(const TransferViewItem *item, int column)
{
    if (column == COLUMN_TRANSFER_FNAME) {
        const QString name = item->data(COLUMN_TRANSFER_FNAME).toString();
        return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(name), 16);
    }
    if (column != COLUMN_TRANSFER_USERS)
        return QVariant();
    return WICON_SIZE(item->download ? AppIcons::eiDOWN : AppIcons::eiUP, 18);
}

QVariant displayText(const TransferViewItem *item, int column)
{
    if (column == COLUMN_TRANSFER_SPEED) {
        const double speed = item->data(COLUMN_TRANSFER_SPEED).toDouble();
        if (speed <= 0.0)
            return QString();
        return WulforUtil::formatBytes(static_cast<int64_t>(speed))
                + TransferViewModel::tr("/s");
    }
    if (column == COLUMN_TRANSFER_SIZE)
        return WulforUtil::formatBytes(item->data(COLUMN_TRANSFER_SIZE).toLongLong());
    if (column == COLUMN_TRANSFER_TLEFT) {
        const int time = item->data(COLUMN_TRANSFER_TLEFT).toInt();
        if (time < 0)
            return QTime(0, 0, 0).toString("hh:mm:ss");
        return QTime(0, 0, 0).addSecs(time).toString("hh:mm:ss");
    }
    return item->data(column);
}

QVariant transferTip(const TransferViewItem *item, int column)
{
    if (column == COLUMN_TRANSFER_FNAME)
        return item->target;
    if (column != COLUMN_TRANSFER_TAG)
        return QVariant();
    const QString tag = item->data(COLUMN_TRANSFER_TAG).toString();
    return tag.isEmpty() ? QVariant() : QVariant(tag);
}

} // namespace

int TransferViewModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootItem->columnCount();
    auto *item = static_cast<TransferViewItem*>(parent.internalPointer());
    return item ? item->columnCount() : 0;
}

QVariant TransferViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TransferViewItem *item = reinterpret_cast<TransferViewItem*>(index.internalPointer());
    // Stale view indexes after layoutChanged/remove can be "valid" with a null pointer;
    // item->download sits at offset 0x10 and crashes QTreeView::indexRowSizeHint on scroll.
    if (!item)
        return QVariant();

    switch (role) {
    case Qt::DecorationRole:
        return transferIcon(item, index.column());
    case Qt::DisplayRole:
        // Single-child groups show the child row (download segments or same-IP uploads).
        if (item->childCount() == 1 && index.column() != COLUMN_TRANSFER_SIZE)
            return data(createIndex(0, index.column(),
                                    reinterpret_cast<void*>(item->childItems.first())), role);
        return displayText(item, index.column());
    case Qt::TextAlignmentRole:
        if (index.column() == COLUMN_TRANSFER_SPEED || index.column() == COLUMN_TRANSFER_SIZE)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::ToolTipRole:
        return transferTip(item, index.column());
    default:
        return QVariant();
    }
}

QVariant TransferViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);
    return QVariant();
}

QModelIndex TransferViewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TransferViewItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else {
        parentItem = static_cast<TransferViewItem*>(parent.internalPointer());
        if (!parentItem)
            return QModelIndex();
    }

    TransferViewItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TransferViewModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TransferViewItem *child = static_cast<TransferViewItem*>(index.internalPointer());
    if (!child)
        return QModelIndex();
    TransferViewItem *parentItem = child->parent();
    if (!parentItem || parentItem == rootItem)
        return QModelIndex();
    if (!rootItem->childItems.contains(parentItem))
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TransferViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    TransferViewItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else {
        parentItem = static_cast<TransferViewItem*>(parent.internalPointer());
        if (!parentItem)
            return 0;
    }
    return parentItem->childCount();
}

bool TransferViewModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootItem->childCount() > 0;

    TransferViewItem *parentItem = static_cast<TransferViewItem*>(parent.internalPointer());
    if (!parentItem)
        return false;
    // Single-child groups stay flat; multi-hub same-IP uploads expand like download sources.
    return parentItem->childCount() > 1;
}

QModelIndex TransferViewModel::createIndexForItem(TransferViewItem *item)
{
    if (!(rootItem && item && item->parent()))
        return QModelIndex();

    if (item->parent() == rootItem)
        return index(item->row(), COLUMN_TRANSFER_FNAME, QModelIndex());
    return index(item->row(), COLUMN_TRANSFER_FNAME,
                 index(item->parent()->row(), COLUMN_TRANSFER_FNAME, QModelIndex()));
}
