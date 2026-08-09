/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "transferrow/TransferViewItem.h"

TransferViewItem::TransferViewItem(const QList<QVariant> &data, TransferViewItem *parent) :
    download(false),
    fail(false),
    finished(false),
    dpos(0L),
    fpos(0L),
    segBytes(0L),
    speedStart(0),
    speedBase(0L),
    percent(0.0),
    smoothTleft(-1),
    finishRank(0),
    itemData(data),
    parentItem(parent)
{
}

TransferViewItem::~TransferViewItem()
{
    qDeleteAll(childItems);
    childItems.clear();
    parentItem = nullptr;
}

void TransferViewItem::appendChild(TransferViewItem *item) {
    item->parentItem = this;
    childItems.append(item);
}

TransferViewItem *TransferViewItem::child(int row) {
    return childItems.value(row);
}

int TransferViewItem::childCount() const {
    return childItems.count();
}

int TransferViewItem::columnCount() const {
    return itemData.count();
}

QVariant TransferViewItem::data(int column) const {
    return itemData.value(column);
}

TransferViewItem *TransferViewItem::parent() {
    return parentItem;
}

int TransferViewItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TransferViewItem*>(this));

    return -1;
}

void TransferViewItem::updateColumn(int column, QVariant var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}
