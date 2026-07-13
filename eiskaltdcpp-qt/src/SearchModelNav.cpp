/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"

int SearchModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootItem->columnCount();
    auto *item = static_cast<SearchItem*>(parent.internalPointer());
    return item ? item->columnCount() : 0;
}

Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    SearchItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else {
        parentItem = static_cast<SearchItem*>(parent.internalPointer());
        if (!parentItem)
            return QModelIndex();
    }

    SearchItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex SearchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SearchItem *childItem = static_cast<SearchItem*>(index.internalPointer());
    if (!childItem)
        return QModelIndex();
    SearchItem *parentItem = childItem->parent();

    if (!parentItem || parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool SearchModel::hasChildren(const QModelIndex &parent) const{
    if (!parent.isValid())
        return true;
    auto *item = static_cast<SearchItem*>(parent.internalPointer());
    return item && item->childCount() > 0;
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    SearchItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else {
        parentItem = static_cast<SearchItem*>(parent.internalPointer());
        if (!parentItem)
            return 0;
    }

    return parentItem->childCount();
}

QModelIndex SearchModel::createIndexForItem(SearchItem *item){
    if (!(rootItem && item) || item == rootItem)
        return QModelIndex();

    // Column 0: rowCount()/hasChildren() only report children for column 0,
    // so branch expand icons require a column-0 parent index.
    return createIndex(item->row(), 0, item);
}
