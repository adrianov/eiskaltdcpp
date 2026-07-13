/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "FavoriteHubModel.h"

bool FavoriteHubModel::insertRow(int row, const QModelIndex &parent){
    Q_UNUSED(row)
    Q_UNUSED(parent)
    return true;
}

bool FavoriteHubModel::removeRow(int row, const QModelIndex &parent){
    Q_UNUSED(row)
    Q_UNUSED(parent)
    return true;
}

bool FavoriteHubModel::insertRows(int position, int rows, const QModelIndex &index){
    Q_UNUSED(rows)

    FavoriteHubItem *from = nullptr;

    beginRemoveRows(QModelIndex(), position, position);
    {
        from = rootItem->childItems.takeAt(position);
    }
    endRemoveRows();

    beginInsertRows(QModelIndex(), index.row(), index.row());
    {
        rootItem->childItems.insert(index.row(), from);
    }
    endInsertRows();

    return true;
}

bool FavoriteHubModel::removeRows(int position, int rows, const QModelIndex &index){
    Q_UNUSED(position)
    Q_UNUSED(rows)
    Q_UNUSED(index)
    return true;
}

void FavoriteHubModel::clearModel(){
    QList<FavoriteHubItem*> childs = rootItem->childItems;
    rootItem->childItems.clear();

    qDeleteAll(childs);

    rootItem->childItems.clear();

    reset();

    emit layoutChanged();
}

void FavoriteHubModel::reset() {
    beginResetModel();
    endResetModel();
}

bool FavoriteHubModel::removeItem(const QModelIndex &el){
    if (!el.isValid())
        return false;

    FavoriteHubItem *item = static_cast<FavoriteHubItem*>(el.internalPointer());

    if (!item || !rootItem->childItems.contains(item))
        return false;

    beginRemoveRows(QModelIndex(), item->row(), item->row());
    rootItem->childItems.removeAt(rootItem->childItems.indexOf(item));
    endRemoveRows();

    delete item;

    emit layoutChanged();

    return true;
}

bool FavoriteHubModel::removeItem(const FavoriteHubItem *item){
    if (!item || !rootItem->childItems.contains(const_cast<FavoriteHubItem*>(item)))
        return false;

    QModelIndex i = index(item->row(), 0, QModelIndex());

    removeItem(i);

    return true;
}

void FavoriteHubModel::addResult(QList<QVariant> &data){
    FavoriteHubItem *item = new FavoriteHubItem(data, rootItem);
    rootItem->appendChild(item);

    emit layoutChanged();
}

QModelIndex FavoriteHubModel::moveDown(const QModelIndex &index){
    if (index.row() >= rootItem->childCount() - 1)
        return QModelIndex();

    FavoriteHubItem *item = nullptr;

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    {
        item = rootItem->childItems.takeAt(index.row());
    }
    endRemoveRows();

    beginInsertRows(QModelIndex(), index.row()+1, index.row()+1);
    {
        rootItem->childItems.insert(index.row()+1, item);
    }
    endInsertRows();

    return this->index(index.row()+1, index.column(), QModelIndex());
}

QModelIndex FavoriteHubModel::moveUp(const QModelIndex &index){
    if (index.row() < 1)
        return QModelIndex();

    FavoriteHubItem *item = nullptr;

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    {
        item = rootItem->childItems.takeAt(index.row());
    }
    endRemoveRows();

    beginInsertRows(QModelIndex(), index.row()-1, index.row()-1);
    {
        rootItem->childItems.insert(index.row()-1, item);
    }
    endInsertRows();

    return this->index(index.row()-1, index.column(), QModelIndex());
}

const QList<FavoriteHubItem*> &FavoriteHubModel::getItems(){
    return rootItem->childItems;
}

void FavoriteHubModel::repaint(){
    emit layoutChanged();
}

Qt::DropActions FavoriteHubModel::supportedDropActions() const{
    return Qt::MoveAction;
}
