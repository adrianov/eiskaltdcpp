/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersItem.h"

#include <QtGlobal>

FinishedTransfersItem::FinishedTransfersItem(const QList<QVariant> &data, FinishedTransfersItem *parent) :
    itemData(data),
    parentItem(parent)
{
}

FinishedTransfersItem::~FinishedTransfersItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void FinishedTransfersItem::appendChild(FinishedTransfersItem *item) {
    childItems.append(item);
}

FinishedTransfersItem *FinishedTransfersItem::child(int row) {
    return ((row >= 0 && row <= childItems.count()-1)? childItems.value(row) : nullptr);
}

int FinishedTransfersItem::childCount() const {
    return childItems.count();
}

int FinishedTransfersItem::columnCount() const {
    return itemData.count();
}

QVariant FinishedTransfersItem::data(int column) const {
    return itemData.value(column);
}

FinishedTransfersItem *FinishedTransfersItem::parent() const{
    return parentItem;
}

int FinishedTransfersItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FinishedTransfersItem*>(this));

    return 0;
}

void FinishedTransfersItem::updateColumn(int column, QVariant var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}
