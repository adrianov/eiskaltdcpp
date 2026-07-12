/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"
#include "SearchLocalPath.h"

SearchItem::SearchItem(const QList<QVariant> &data, SearchItem *parent) :
    count(0),
    isDir(false),
    itemData(data),
    parentItem(parent)
{
}

SearchItem::~SearchItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void SearchItem::appendChild(SearchItem *item) {
    childItems.append(item);
    count = childItems.size();
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
        return childItems.size()+1;

    return itemData.value(column);
}

SearchItem *SearchItem::parent() const{
    return parentItem;
}

int SearchItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<SearchItem*>(this));

    return 0;
}

bool SearchItem::exists(const QString &user_cid) const {
    if (childItems.isEmpty())
        return cid == user_cid;

    for (const auto &child : childItems) {
        if (child->cid == user_cid)
            return true;
    }
    return false;
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

SearchListException::SearchListException() :
    message("Unknown"), type(Unkn)
{}

SearchListException::SearchListException(const SearchListException &ex) :
    message(ex.message), type(ex.type)
{}

SearchListException::SearchListException(const QString& message, Type type) :
    message(message), type(type)
{}

SearchListException::~SearchListException(){
}

SearchListException &SearchListException::operator =(const SearchListException &ex2) {
    type = ex2.type;
    message = ex2.message;

    return (*this);
}
