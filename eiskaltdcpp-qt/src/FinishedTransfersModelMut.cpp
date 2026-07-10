/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersModel.h"
#include "FinishedTransfersProxy.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"

using namespace dcpp;

bool FinishedTransfersModel::acceptDownloadFile(const QVariantMap &params) const {
    if (!hideFileLists && !requireFullFile)
        return true;

    const string target = _tq(params["TARGET"].toString());
    const string path = _tq(params["PATH"].toString() + params["FNAME"].toString());

    if (hideFileLists && (isFinishedFileList(target) || isFinishedFileList(path)))
        return false;

    if (requireFullFile && !params["FULL"].toBool())
        return false;

    return true;
}

void FinishedTransfersModel::addFile(const QVariantMap &params){
    if (!acceptDownloadFile(params))
        return;

    FinishedTransfersItem *item = findFile(params["TARGET"].toString());

    if (!item)
        return;

    for (int i = 0; i < fileItem->columnCount(); i++){
        if (file_header_table[i] == "USERS"){
            QStringList users = params[file_header_table[i]].toString().split(" ");
            QStringList old_users = item->data(i).toString().split(" ");

            if (users.isEmpty())
                continue;
            else{
                for (const auto nick : users){
                    if (!old_users.contains(nick))
                        old_users.push_back(nick);
                }

                item->updateColumn(i, old_users.join(" "));
            }
        }
        else
            item->updateColumn(i, params[file_header_table[i]]);
    }

    if (bulkLoadDepth == 0 && rootItem == fileItem)
        sort();
    else
        emit dataChanged(createIndex(item->row(), COLUMN_FINISHED_NAME, item), createIndex(item->row(), COLUMN_FINISHED_FULL, item));
}

void FinishedTransfersModel::addUser(const QVariantMap &params){
    FinishedTransfersItem *item = findUser(params["CID"].toString());

    if (!item)
        return;

    for (int i = 0; i < userItem->columnCount(); i++){
        if (user_header_table[i] == "NICK"){
            QString user = params[user_header_table[i]].toString();

            if (user.trimmed().isEmpty() || user.trimmed().isNull())
                continue;
            else
                item->updateColumn(i, user);
        }
        else
            item->updateColumn(i, params[user_header_table[i]]);
    }

    if (bulkLoadDepth == 0 && rootItem == userItem)
        sort();
    else
        emit dataChanged(createIndex(item->row(), COLUMN_FINISHED_NAME, item), createIndex(item->row(), COLUMN_FINISHED_CRC32, item));
}

void FinishedTransfersModel::remFile(const QString &file){
    auto it = file_hash.find(file);
    if (it == file_hash.end())
        return;

    FinishedTransfersItem *item = it.value();
    file_hash.erase(it);

    beginRemoveRows(QModelIndex(), item->row(), item->row());
    {
        fileItem->childItems.removeAt(item->row());

        delete item;
    }
    endRemoveRows();
}

void FinishedTransfersModel::remUser(const QString &cid){
    auto it = user_hash.find(cid);
    if (it == user_hash.end())
        return;

    FinishedTransfersItem *item = it.value();
    user_hash.erase(it);

    beginRemoveRows(QModelIndex(), item->row(), item->row());
    {
        userItem->childItems.removeAt(item->row());

        delete item;
    }
    endRemoveRows();
}

void FinishedTransfersModel::switchViewType(FinishedTransfersModel::ViewType t){
    beginResetModel();
    switch (t){
        case FileView:
            rootItem = fileItem;
            break;
        case UserView:
            rootItem = userItem;
            break;
    }
    endResetModel();
    sort();
}
