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

#include "search/SearchItem.h"
#include "search/SearchLocalPath.h"

#include <QtAlgorithms>

SearchItem::SearchItem(const QList<QVariant> &data, SearchItem *parent) :
    isDir(false),
    itemData(data),
    parentItem(parent)
{
}

SearchItem::~SearchItem()
{
    clearChildren();
}

void SearchItem::appendChild(SearchItem *item) {
    insertChild(childItems.size(), item);
}

void SearchItem::insertChild(int row, SearchItem *item) {
    childItems.insert(row, item);
    sources.invalidate();
}

void SearchItem::removeChild(int row) {
    childItems.removeAt(row);
    sources.invalidate();
}

void SearchItem::clearChildren() {
    qDeleteAll(childItems);
    childItems.clear();
    sources.invalidate();
}

SearchItem *SearchItem::child(int row) {
    return childItems.value(row);
}

int SearchItem::childCount() const {
    return childItems.count();
}

int SearchItem::columnCount() const {
    return itemData.count();
}

QVariant SearchItem::data(int column) const {
    if (column == COLUMN_SF_COUNT && !childItems.isEmpty() && parentItem)
        return sources.uniqueCount(this);
    if (column == COLUMN_SF_ONLINE && parentItem && !parentItem->parent())
        return sources.onlineCount(this);
    return itemData.value(column);
}

void SearchItem::clearOnlineCount() {
    sources.invalidateOnline();
}

void SearchItem::updateColumn(int column, const QVariant &value) {
    if (column < 0 || column >= itemData.size())
        return;
    itemData[column] = value;
}

SearchItem *SearchItem::parent() const{
    return parentItem;
}

int SearchItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<SearchItem*>(this));

    return 0;
}

SearchItem *SearchItem::findSource(const QString &user_cid) {
    if (cid == user_cid)
        return this;
    for (SearchItem *child : childItems) {
        if (child->cid == user_cid)
            return child;
    }
    return nullptr;
}

QString SearchItem::localPath() const {
    if (localChecked)
        return localCached;

    localChecked = true;
    if (!isDir)
        localCached = SearchLocalPath::resolve(data(COLUMN_SF_TTH).toString(),
                                               data(COLUMN_SF_ESIZE).toLongLong());
    return localCached;
}

void SearchItem::clearLocalPath() {
    localChecked = false;
    localCached.clear();
}

bool SearchItem::isQueued() const {
    if (queuedChecked)
        return queuedCached;

    queuedChecked = true;
    if (!isDir)
        queuedCached = SearchLocalPath::isQueued(data(COLUMN_SF_TTH).toString());
    return queuedCached;
}

void SearchItem::clearQueued() {
    queuedChecked = false;
    queuedCached = false;
}
