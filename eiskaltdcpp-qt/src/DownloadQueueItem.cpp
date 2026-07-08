/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "DownloadQueueModel.h"

DownloadQueueItem::DownloadQueueItem(const QList<QVariant> &data, DownloadQueueItem *parent)
    : dir(false)
    , itemData(data)
    , parentItem(parent)
{
}

DownloadQueueItem::DownloadQueueItem(const DownloadQueueItem &in)
    : dir(in.dir)
    , itemData(in.itemData)
    , parentItem(nullptr)
{
}

void DownloadQueueItem::operator=(const DownloadQueueItem &in){
    parentItem = in.parentItem;
    childItems = in.childItems;
    itemData = in.itemData;
    dir = in.dir;
}

DownloadQueueItem::~DownloadQueueItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void DownloadQueueItem::appendChild(DownloadQueueItem *item) {
    childItems.append(item);
    item->parentItem = this;
}

DownloadQueueItem *DownloadQueueItem::child(int row) {
    return childItems.value(row);
}

int DownloadQueueItem::childCount() const {
    return childItems.count();
}

int DownloadQueueItem::columnCount() const {
    return itemData.count();
}

const QVariant &DownloadQueueItem::data(int column) const {
    return itemData.at(column);
}

DownloadQueueItem *DownloadQueueItem::parent() {
    return parentItem;
}

int DownloadQueueItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<DownloadQueueItem*>(this));

    return 0;
}

void DownloadQueueItem::updateColumn(int column, QVariant var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}

DownloadQueueItem *DownloadQueueItem::nextSibling(){
    if (!parent())
        return nullptr;

    if (row() == (parent()->childCount()-1))
        return nullptr;

    return parent()->child(row()+1);
}
