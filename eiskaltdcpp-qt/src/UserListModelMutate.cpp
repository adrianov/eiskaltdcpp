/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "UserListModel.h"
#include "UserListModelSort.h"

#include "dcpp/stdinc.h"

void UserListModel::updateUser(UserListItem *item, const Identity& _id, const QString& _cid, bool _fav) {
    if (!item || item->parent() != rootItem)
        return;

    bool needSorted = (item->getIdentity().isOp() != _id.isOp()) || (item->isFav() != _fav);

    if (sortColumn != -1) {
        switch (sortColumn) {
            case COLUMN_NICK:
                needSorted = needSorted || (item->getIdentity().getNick() != _id.getNick());
                break;
            case COLUMN_SHARE:
            case COLUMN_EXACT_SHARE:
                needSorted = needSorted || (item->getIdentity().getBytesShared() != _id.getBytesShared());
                break;
            case COLUMN_COMMENT:
                needSorted = needSorted || (item->getIdentity().getDescription() != _id.getDescription());
                break;
            case COLUMN_TAG:
                needSorted = needSorted || (item->getIdentity().getTag() != _id.getTag());
                break;
            case COLUMN_CONN:
                needSorted = needSorted || (item->getIdentity().getConnection() != _id.getConnection());
                break;
            case COLUMN_IP:
                needSorted = needSorted || (item->getIdentity().getIp() != _id.getIp());
                break;
            case COLUMN_EMAIL:
                needSorted = needSorted || (item->getIdentity().getEmail() != _id.getEmail());
                break;
        }
    }

    item->updateIdentity(_id, _cid, _fav);

    if (needSorted) {
        const int oldRow = item->row();

        beginRemoveRows(QModelIndex(), oldRow, oldRow);
        {
            rootItem->childItems.removeAt(oldRow);
        }
        endRemoveRows();

        auto it = userListInsertSorted(rootItem->childItems, item, sortColumn, sortOrder);
        const int newRow = it - rootItem->childItems.begin();

        beginInsertRows(QModelIndex(), newRow, newRow);
        {
            rootItem->childItems.insert(it, item);
        }
        endInsertRows();
    } else {
        repaintData(index(item->row(), COLUMN_NICK), index(item->row(), COLUMN_EMAIL));
    }

    return;
}

UserListItem *UserListModel::addUser(const UserPtr& _ptr, const Identity& _id, const QString& _cid, bool _fav) {

    if (users.contains(_ptr))
        return itemForPtr(_ptr);

    UserListItem *item = new UserListItem(rootItem, _ptr, _id, _cid, _fav);

    users.insert(_ptr, item);

    if (sortColumn == -1) // if sorting disabled
    {
        const int row = rootItem->childCount();

        beginInsertRows(QModelIndex(), row, row);
        {
            rootItem->appendChild(item);
        }
        endInsertRows();

    } else {
        auto it = userListInsertSorted(rootItem->childItems, item, sortColumn, sortOrder);
        const int row = it - rootItem->childItems.begin();

        beginInsertRows(QModelIndex(), row, row);
        {
            rootItem->childItems.insert(it, item);
        }
        endInsertRows();
    }

    return item;
}

