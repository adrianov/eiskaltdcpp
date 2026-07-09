/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersModel.h"

#include <QList>

FinishedTransfersModel::FinishedTransfersModel(QObject *parent):
        QAbstractItemModel(parent), sortColumn(COLUMN_FINISHED_TIME), sortOrder(Qt::AscendingOrder),
        hideFileLists(false), requireFullFile(false), bulkLoadDepth(0)
{
    QList<QVariant> userData;
    userData << tr("User")<< tr("Files") << tr("Time") << tr("Transferred")
             << tr("Speed") << tr("Elapsed time") << tr("Full");

    userItem = new FinishedTransfersItem(userData);

    QList<QVariant> fileData;
    fileData << tr("Filename") << tr("Path") << tr("Time") << tr("User")
             << tr("Transferred") << tr("Speed")   << tr("Check sum")
             << tr("Target") << tr("Elapsed time") << tr("Full");

    fileItem = new FinishedTransfersItem(fileData);

    rootItem = fileItem;

    file_header_table.insert(COLUMN_FINISHED_NAME,      "FNAME");
    file_header_table.insert(COLUMN_FINISHED_PATH,      "PATH");
    file_header_table.insert(COLUMN_FINISHED_TIME,      "TIME");
    file_header_table.insert(COLUMN_FINISHED_USER,      "USERS");
    file_header_table.insert(COLUMN_FINISHED_TR,        "TR");
    file_header_table.insert(COLUMN_FINISHED_SPEED,     "SPEED");
    file_header_table.insert(COLUMN_FINISHED_CRC32,     "CRC32");
    file_header_table.insert(COLUMN_FINISHED_TARGET,    "TARGET");
    file_header_table.insert(COLUMN_FINISHED_ELAPS,     "ELAP");
    file_header_table.insert(COLUMN_FINISHED_FULL,      "FULL");

    user_header_table.insert(COLUMN_FINISHED_NAME,      "NICK");
    user_header_table.insert(COLUMN_FINISHED_PATH,      "FILES");
    user_header_table.insert(COLUMN_FINISHED_TIME,      "TIME");
    user_header_table.insert(COLUMN_FINISHED_USER,      "TR");
    user_header_table.insert(COLUMN_FINISHED_TR,        "SPEED");
    user_header_table.insert(COLUMN_FINISHED_SPEED,     "ELAP");
    user_header_table.insert(COLUMN_FINISHED_CRC32,     "FULL");
}

FinishedTransfersModel::~FinishedTransfersModel()
{
    delete userItem;
    delete fileItem;
}

FinishedTransfersItem *FinishedTransfersModel::findFile(const QString &fname){
    if (fname.isEmpty())
        return nullptr;

    auto it = file_hash.find(fname);

    if (it != file_hash.constEnd())
        return const_cast<FinishedTransfersItem*>(it.value());

    FinishedTransfersItem *item = new FinishedTransfersItem(QList<QVariant>() << "" << "" << "" << ""
                                                                              << "" << "" << "" << ""
                                                                              << "" << false,
                                                            fileItem);
    if (fileItem == rootItem){
        emit beginInsertRows(QModelIndex(), rootItem->childCount(), rootItem->childCount());
        fileItem->appendChild(item);
        emit endInsertRows();
    }
    else
        fileItem->appendChild(item);

    file_hash.insert(fname, item);
    return item;
}

FinishedTransfersItem *FinishedTransfersModel::findUser(const QString &cid){
    if (cid.isEmpty())
        return nullptr;

    auto it = user_hash.find(cid);

    if (it != user_hash.constEnd())
        return const_cast<FinishedTransfersItem*>(it.value());

    FinishedTransfersItem *item = new FinishedTransfersItem(QList<QVariant>() << "" << "" << ""
                                                                              << "" << "" << ""
                                                                              << false,
                                                            userItem);
    if (userItem == rootItem){
        emit beginInsertRows(QModelIndex(), rootItem->childCount(), rootItem->childCount());
        userItem->appendChild(item);
        emit endInsertRows();
    }
    else
        userItem->appendChild(item);

    user_hash.insert(cid, item);
    return item;
}

void FinishedTransfersModel::repaint(){
    emit layoutChanged();
}
