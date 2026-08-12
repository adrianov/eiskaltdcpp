/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "TransferViewModelTree.h"

TransferViewModel::TransferViewModel(QObject *parent)
    : QAbstractItemModel(parent), grace(this), showTranferedFilesOnly(false)
{
    QList<QVariant> rootData;
    rootData << tr("Users") << tr("Speed") << tr("Status") << tr("Flags") << tr("Size")
             << tr("Time left") << tr("File name") << tr("Host") << tr("Tag") << tr("IP")
             << tr("Encryption");

    rootItem = new TransferViewItem(rootData, nullptr);

    column_map.insert("USER", COLUMN_TRANSFER_USERS);
    column_map.insert("SPEED", COLUMN_TRANSFER_SPEED);
    column_map.insert("STAT", COLUMN_TRANSFER_STATS);
    column_map.insert("FLAGS", COLUMN_TRANSFER_FLAGS);
    column_map.insert("ESIZE", COLUMN_TRANSFER_SIZE);
    column_map.insert("TLEFT", COLUMN_TRANSFER_TLEFT);
    column_map.insert("FNAME", COLUMN_TRANSFER_FNAME);
    column_map.insert("HOST", COLUMN_TRANSFER_HOST);
    column_map.insert("TAG", COLUMN_TRANSFER_TAG);
    column_map.insert("IP", COLUMN_TRANSFER_IP);
    column_map.insert("ENCRYPTION", COLUMN_TRANSFER_ENCRYPTION);

    sortColumn = COLUMN_TRANSFER_SIZE;
    sortOrder = Qt::DescendingOrder;

    connect(&grace, &TransferGrace::uploadDue, this, &TransferViewModel::pruneUpload);
    connect(&grace, &TransferGrace::downloadDue, this, &TransferViewModel::pruneDownload);
}

TransferViewModel::~TransferViewModel()
{
    // forgetItem only drops liveItems entries — it does not free rows.
    liveItems.clear();
    delete rootItem;
    rootItem = nullptr;
}

void TransferViewModel::trackItem(TransferViewItem *item)
{
    if (item)
        liveItems.insert(item);
}

void TransferViewModel::forgetItem(TransferViewItem *item)
{
    if (!item || !liveItems.contains(item))
        return;
    const QList<TransferViewItem*> children = item->childItems;
    for (TransferViewItem *child : children)
        forgetItem(child);
    liveItems.remove(item);
}

void TransferViewModel::destroyRow(TransferViewItem *item)
{
    if (!item)
        return;
    forgetItem(item);
    delete item;
}

bool TransferViewModel::isLive(TransferViewItem *item) const
{
    return item && liveItems.contains(item);
}

TransferViewItem *TransferViewModel::liveChild(TransferViewItem *parent, int row) const
{
    if (!parent || (parent != rootItem && !isLive(parent)))
        return nullptr;
    TransferViewItem *child = parent->child(row);
    return isLive(child) ? child : nullptr;
}

QModelIndex TransferViewModel::indexOfItem(TransferViewItem *item) const
{
    if (!isLive(item) || item == rootItem)
        return QModelIndex();
    TransferViewItem *parentItem = item->parent();
    if (!parentItem)
        return QModelIndex();
    return createIndex(item->row(), 0, item);
}

void TransferViewModel::insertUnder(TransferViewItem *parent, TransferViewItem *item)
{
    if (!item || !parent || TransferViewTree::isAttached(item))
        return;

    trackItem(item);
    const QModelIndex parentIdx = (parent == rootItem) ? QModelIndex() : indexOfItem(parent);
    beginInsertRows(parentIdx, parent->childCount(), parent->childCount());
    parent->appendChild(item);
    endInsertRows();
}

void TransferViewModel::clear()
{
    // Reset before free: deleting first leaves QTreeView with dangling indexes.
    beginResetModel();
    liveItems.clear();
    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();
    transfer_hash.clear();
    endResetModel();
}

void TransferViewModel::repaint()
{
    // Identity remap — bare layoutChanged() nulls persistent indexes and crashes scroll.
    const QModelIndexList from = persistentIndexList();
    emit layoutAboutToBeChanged();
    changePersistentIndexList(from, from);
    emit layoutChanged();
}
