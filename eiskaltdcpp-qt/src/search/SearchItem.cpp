/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "search/SearchItem.h"
#include "search/SearchLocalPath.h"

#include <QSet>

namespace {

QString sourceIp(const SearchItem *item) {
    QString ip = item->data(COLUMN_SF_IP).toString().trimmed();
    if (ip == QLatin1String("0.0.0.0"))
        ip.clear();
    return ip;
}

/** Distinct sources: same IP once; nick merges with that IP when present, else alone; else CID. */
int uniqueSourceCount(const SearchItem *item) {
    QSet<QString> ips;
    QSet<QString> nicksWithIp;
    QSet<QString> nicksNoIp;
    QSet<QString> cidsOnly;

    auto add = [&](const SearchItem *it) {
        const QString ip = sourceIp(it);
        const QString nick = it->data(COLUMN_SF_NICK).toString();
        if (!ip.isEmpty()) {
            ips.insert(ip);
            if (!nick.isEmpty())
                nicksWithIp.insert(nick);
        } else if (!nick.isEmpty()) {
            nicksNoIp.insert(nick);
        } else {
            cidsOnly.insert(it->cid);
        }
    };

    add(item);
    for (const SearchItem *child : item->children())
        add(child);

    int n = ips.size() + cidsOnly.size();
    for (const QString &nick : nicksNoIp) {
        if (!nicksWithIp.contains(nick))
            ++n;
    }
    return n;
}

} // namespace

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
    countCached = -1;
}

void SearchItem::removeChild(int row) {
    childItems.removeAt(row);
    countCached = -1;
}

void SearchItem::clearChildren() {
    qDeleteAll(childItems);
    childItems.clear();
    countCached = -1;
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
    if (column == COLUMN_SF_COUNT && !childItems.isEmpty() && parentItem) {
        if (countCached < 0)
            countCached = uniqueSourceCount(this);
        return countCached;
    }

    return itemData.value(column);
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

bool SearchItem::exists(const QString &user_cid) const {
    if (cid == user_cid)
        return true;

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
