/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "queuedusers/QueuedUsersModel.h"

QueuedUsersModel::QueuedUsersModel(QObject *parent) :
    QAbstractItemModel(parent),
    tree(QList<QVariant>() << tr("User") << tr("File"))
{
}

QueuedUserItem *QueuedUsersModel::itemAt(const QModelIndex &index) const {
    return index.isValid() ? static_cast<QueuedUserItem*>(index.internalPointer()) : nullptr;
}

int QueuedUsersModel::columnCount(const QModelIndex &) const {
    return 2;
}

QVariant QueuedUsersModel::data(const QModelIndex &index, int role) const {
    QueuedUserItem *item = itemAt(index);
    if (!item || role != Qt::DisplayRole)
        return QVariant();
    return item->data(index.column());
}

Qt::ItemFlags QueuedUsersModel::flags(const QModelIndex &index) const {
    return index.isValid() ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::ItemFlags();
}

QVariant QueuedUsersModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return tree.root()->data(section);
    return QVariant();
}

QModelIndex QueuedUsersModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QueuedUserItem *parentItem = parent.isValid() ? itemAt(parent) : tree.root();
    QueuedUserItem *childItem = parentItem ? parentItem->child(row) : nullptr;
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex QueuedUsersModel::parent(const QModelIndex &index) const {
    QueuedUserItem *childItem = itemAt(index);
    if (!childItem)
        return QModelIndex();

    QueuedUserItem *parentItem = childItem->parent();
    if (!parentItem || parentItem == tree.root())
        return QModelIndex();
    return createIndex(parentItem->row(), 0, parentItem);
}

int QueuedUsersModel::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0)
        return 0;
    QueuedUserItem *parentItem = parent.isValid() ? itemAt(parent) : tree.root();
    return parentItem ? parentItem->childCount() : 0;
}

void QueuedUsersModel::sort(int column, Qt::SortOrder order) {
    if (!tree.root())
        return;
    if (tree.root()->childItems.empty()) {
        tree.sort(column, order);
        return;
    }

    // Remap persistent indexes across reorder (stale pointer → crash in parent()).
    const QModelIndexList from = persistentIndexList();
    QList<QueuedUserItem*> fromItems;
    fromItems.reserve(from.size());
    for (const QModelIndex &idx : from)
        fromItems.append(static_cast<QueuedUserItem*>(idx.internalPointer()));

    emit layoutAboutToBeChanged();
    tree.sort(column, order);

    QModelIndexList to;
    to.reserve(from.size());
    for (int i = 0; i < fromItems.size(); ++i) {
        QueuedUserItem *item = fromItems.at(i);
        QueuedUserItem *p = item ? item->parent() : nullptr;
        if (!item || !p || !p->childItems.contains(item))
            to.append(QModelIndex());
        else
            to.append(createIndex(item->row(), from.at(i).column(), item));
    }
    changePersistentIndexList(from, to);
    emit layoutChanged();
}

void QueuedUsersModel::addResult(const VarMap &map) {
    if (!map.contains(QLatin1String("CID")))
        return;

    const QString cid = map.value(QLatin1String("CID")).toString();
    if (cid.isEmpty())
        return;

    QueuedUserItem *user = tree.user(cid);
    if (!user) {
        const int row = tree.root()->childCount();
        beginInsertRows(QModelIndex(), row, row);
        user = tree.addUser(map);
        endInsertRows();
    }
    if (!user)
        return;

    const int row = user->childCount();
    beginInsertRows(createIndex(user->row(), 0, user), row, row);
    tree.addFile(user, map);
    endInsertRows();
    sort();
}

void QueuedUsersModel::remResult(const VarMap &map) {
    if (!map.contains(QLatin1String("CID")))
        return;

    const QString cid = map.value(QLatin1String("CID")).toString();
    QueuedUserItem *user = tree.user(cid);
    if (!user)
        return;

    const int row = user->row();
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    delete tree.takeUser(cid);
    endRemoveRows();
}
