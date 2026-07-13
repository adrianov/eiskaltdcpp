/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "FavoriteHubModel.h"

FavoriteHubItem::FavoriteHubItem(const QList<QVariant> &data, FavoriteHubItem *parent) :
    itemData(data), parentItem(parent)
{
}

FavoriteHubItem::~FavoriteHubItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void FavoriteHubItem::appendChild(FavoriteHubItem *item) {
    childItems.append(item);
}

FavoriteHubItem *FavoriteHubItem::child(int row) {
    return childItems.value(row);
}

int FavoriteHubItem::childCount() const {
    return childItems.count();
}

int FavoriteHubItem::columnCount() const {
    return itemData.count();
}

QVariant FavoriteHubItem::data(int column) const {
    return itemData.at(column);
}

FavoriteHubItem *FavoriteHubItem::parent() {
    return parentItem;
}

int FavoriteHubItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FavoriteHubItem*>(this));

    return 0;
}

void FavoriteHubItem::updateColumn(const int column, const QVariant &var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}
