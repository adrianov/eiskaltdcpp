/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubModel.h"

PublicHubItem::PublicHubItem(const QList<QVariant> &data, PublicHubItem *parent) :
    entry(nullptr), itemData(data), parentItem(parent)
{
}

PublicHubItem::~PublicHubItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void PublicHubItem::appendChild(PublicHubItem *item) {
    childItems.append(item);
}

PublicHubItem *PublicHubItem::child(int row) {
    return childItems.value(row);
}

int PublicHubItem::childCount() const {
    return childItems.count();
}

int PublicHubItem::columnCount() const {
    return itemData.count();
}

QVariant PublicHubItem::data(int column) const {
    return itemData.at(column);
}

PublicHubItem *PublicHubItem::parent() {
    return parentItem;
}

int PublicHubItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<PublicHubItem*>(this));

    return 0;
}

void PublicHubItem::updateColumn(const int column, const QVariant &var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}
