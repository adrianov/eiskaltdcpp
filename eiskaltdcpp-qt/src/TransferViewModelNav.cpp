/***************************************************************************
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

    switch(role) {
        case Qt::DecorationRole:
        {
            if (index.column() != COLUMN_TRANSFER_USERS && index.column() != COLUMN_TRANSFER_FNAME)
                break;

            if (item->download && index.column() == COLUMN_TRANSFER_USERS)
                return WICON_SIZE(WulforUtil::eiDOWN, 18);
            else if (index.column() != COLUMN_TRANSFER_FNAME)
                return WICON_SIZE(WulforUtil::eiUP, 18);
            else
                return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_TRANSFER_FNAME).toString()), 16);
        }
        case Qt::DisplayRole:
        {
            // Single-child groups show the child row (download segments or same-IP uploads).
            if (item->childCount() == 1 && index.column() != COLUMN_TRANSFER_SIZE)
                return data(createIndex(0, index.column(), reinterpret_cast<void*>(item->childItems.first())), role);

            if (index.column() == COLUMN_TRANSFER_SPEED)
                return WulforUtil::formatDisplayBytes(static_cast<int64_t>(item->data(COLUMN_TRANSFER_SPEED).toDouble())) + tr("/s");
            else if (index.column() == COLUMN_TRANSFER_SIZE)
                return WulforUtil::formatDisplayBytes(item->data(COLUMN_TRANSFER_SIZE).toLongLong());
            else if (index.column() == COLUMN_TRANSFER_TLEFT){
                const int time = item->data(COLUMN_TRANSFER_TLEFT).toInt();
                if (time < 0)
                    return QTime(0, 0, 0).toString("hh:mm:ss");
                return QTime(0, 0, 0).addSecs(time).toString("hh:mm:ss");
            }

            return item->data(index.column());
        }
        case Qt::TextAlignmentRole:
            if (index.column() == COLUMN_TRANSFER_SPEED || index.column() == COLUMN_TRANSFER_SIZE)
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        case Qt::ToolTipRole:
            if (index.column() == COLUMN_TRANSFER_FNAME)
                return item->target;
            break;
        default:
            break;
    }

    return QVariant();
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
