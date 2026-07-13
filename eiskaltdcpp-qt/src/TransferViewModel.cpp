/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"

TransferViewModel::TransferViewModel(QObject *parent)
    : QAbstractItemModel(parent), showTranferedFilesOnly(false)
{
    QList<QVariant> rootData;
    rootData << tr("Users") << tr("Speed") << tr("Status") << tr("Flags") << tr("Size")
             << tr("Time left") << tr("File name") << tr("Host") << tr("IP")
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
    column_map.insert("IP", COLUMN_TRANSFER_IP);
    column_map.insert("ENCRYPTION", COLUMN_TRANSFER_ENCRYPTION);

    sortColumn = COLUMN_TRANSFER_SIZE;
    sortOrder = Qt::DescendingOrder;
}

TransferViewModel::~TransferViewModel()
{
    if (rootItem)
        delete rootItem;
}

void TransferViewModel::clear()
{
    // Reset before free: deleting first leaves QTreeView with dangling indexes.
    beginResetModel();
    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();
    endResetModel();
}

void TransferViewModel::repaint()
{
    emit layoutChanged();
}
