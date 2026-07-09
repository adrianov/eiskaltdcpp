/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "UserListModel.h"
#include "WulforUtil.h"

#include <algorithm>

void UserListProxyModel::sort(int column, Qt::SortOrder order){
    if (sourceModel())
        sourceModel()->sort(column, order);
}

UserListModel::UserListModel(QObject * parent)
    : QAbstractItemModel(parent)
    , rootItem(new UserListItem())
    , sortColumn(COLUMN_SHARE)
    , sortOrder(Qt::DescendingOrder)
    , WU(WulforUtil::getInstance())
{
    stripper.setPattern("\\[.*\\]");
    stripper.setMinimal(true);
}


UserListModel::~UserListModel() {
    delete rootItem;
}


int UserListModel::rowCount(const QModelIndex & ) const {
    return rootItem->childCount();
}

int UserListModel::columnCount(const QModelIndex & ) const {
    return 8;
}

bool UserListModel::hasChildren(const QModelIndex &parent) const{
    return (!parent.isValid());
}

bool UserListModel::canFetchMore(const QModelIndex &parent) const{
    Q_UNUSED(parent)
    return false;
}

QModelIndex UserListModel::index(int row, int column, const QModelIndex &) const {
    if (row > (rootItem->childCount() - 1) || row < 0)
        return QModelIndex();

    return createIndex(row, column, rootItem->child(row));
}

QModelIndex UserListModel::parent(const QModelIndex & ) const {
    return QModelIndex();
}

void UserListModel::clear() {
    emit layoutAboutToBeChanged();

    users.clear();

    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();

    emit layoutChanged();
}

void UserListModel::removeUser(const UserPtr &ptr) {
    auto iter = users.find(ptr);

    if (iter == users.end())
        return;

    const int index = (iter.value())->row();

    beginRemoveRows(QModelIndex(), index, index);

    UserListItem *item = iter.value();

    rootItem->childItems.removeAt(index);
    delete item;

    users.erase(iter);

    endRemoveRows();
}

UserListItem *UserListModel::itemForPtr(const UserPtr &ptr){
    auto iter = users.find(ptr);

    return (iter != users.end())? iter.value() : nullptr;
}

UserListItem *UserListModel::itemForNick(const QString &nick, const QString &){   
    auto u = std::find_if(rootItem->childItems.begin(), rootItem->childItems.end(),
                                                    [&nick](UserListItem *i) -> bool
                                                    {
                                                        return (i->getNick() == nick);
                                                    }
                                                    );

    return (u == rootItem->childItems.end())? nullptr : *u;
}

QString UserListModel::CIDforNick(const QString &nick, const QString &){
    UserListItem *item = itemForNick(nick, "");
    return (item? item->getCID() : "");
}

void UserListModel::repaintItem(const UserListItem *item){
    int r = rootItem->childItems.indexOf(const_cast<UserListItem*>(item));

    if (!(item && r >= 0))
        return;

    repaintData(createIndex(r, COLUMN_NICK, const_cast<UserListItem*>(item)), createIndex(r, COLUMN_EMAIL, const_cast<UserListItem*>(item)));
}
