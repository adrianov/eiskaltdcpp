/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "QueuedUsers.h"

QueuedUserItem::QueuedUserItem(const QList<QVariant> &data, QueuedUserItem *parent) :
    itemData(data),
    parentItem(parent)
{
}

QueuedUserItem::~QueuedUserItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void QueuedUserItem::appendChild(QueuedUserItem *item) {
    childItems.append(item);
}

QueuedUserItem *QueuedUserItem::child(int row) {
    return childItems.value(row);
}

int QueuedUserItem::childCount() const {
    return childItems.count();
}

int QueuedUserItem::columnCount() const {
    return itemData.count();
}

QVariant QueuedUserItem::data(int column) const {
    return itemData.value(column);
}

QueuedUserItem *QueuedUserItem::parent() const{
    return parentItem;
}

int QueuedUserItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<QueuedUserItem*>(this));

    return 0;
}
