/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "QueuedUsers.h"
#include "QueuedUsersSort.h"
#include "WulforUtil.h"

QueuedUsersModel::QueuedUsersModel(QObject *parent):
        QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << tr("User") << tr("File");

    rootItem = new QueuedUserItem(rootData);
}

QueuedUsersModel::~QueuedUsersModel()
{
    delete rootItem;
}

int QueuedUsersModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant QueuedUsersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QueuedUserItem *item = static_cast<QueuedUserItem*>(index.internalPointer());

    switch(role) {
        case Qt::DisplayRole:
            return item->data(index.column());
        default:
            break;
    }

    return QVariant();
}

Qt::ItemFlags QueuedUsersModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant QueuedUsersModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex QueuedUsersModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QueuedUserItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QueuedUserItem*>(parent.internalPointer());

    QueuedUserItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QueuedUsersModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QueuedUserItem *childItem = static_cast<QueuedUserItem*>(index.internalPointer());
    QueuedUserItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int QueuedUsersModel::rowCount(const QModelIndex &parent) const
{
    QueuedUserItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QueuedUserItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void QueuedUsersModel::sort(int column, Qt::SortOrder order) {
    static int sortColumn = 0;
    static Qt::SortOrder sortOrder = Qt::AscendingOrder;

    sortColumn = (column < 0 || column > 1)? sortColumn : column;
    sortOrder = (column < 0 || column > 1)? sortOrder : order;

    emit layoutAboutToBeChanged();
    sortQueuedUserItems(sortColumn, sortOrder, rootItem->childItems);
    emit layoutChanged();
}

void QueuedUsersModel::addResult(const VarMap &map)
{
    if (!map.contains("CID"))
        return;

    const QString &cid  = map["CID"].toString();
    const QString &file = map["FILE"].toString();
    const QString &hub  = map["HUB"].toString();
    auto it = cids.find(cid);
    QueuedUserItem *parentItem = nullptr;

    if (it != cids.end())
        parentItem = it.value();
    else {
        parentItem = new QueuedUserItem(QList<QVariant>() << WulforUtil::getInstance()->getNicks(cid) << "", rootItem);
        parentItem->cid     = cid;
        parentItem->file    = file;
        parentItem->hub     = hub;

        rootItem->appendChild(parentItem);

        cids.insert(cid, parentItem);
    }

    if (!parentItem)
        return;

    QueuedUserItem *item = new QueuedUserItem(QList<QVariant>() << "" << file, parentItem);
    item->cid     = cid;
    item->file    = file;
    item->hub     = hub;

    parentItem->appendChild(item);

    sort();
}

void QueuedUsersModel::remResult(const VarMap &map){
    if (!map.contains("CID"))
        return;

    const QString &cid  = map["CID"].toString();
    auto it = cids.find(cid);
    QueuedUserItem *parentItem = nullptr;

    if (it != cids.end())
        parentItem = it.value();
    else
        return;

    if (!parentItem)
        return;

    emit layoutChanged();

    parentItem->parent()->childItems.removeAt(parentItem->row());
    cids.remove(cid);

    delete parentItem;

    emit layoutChanged();
}
