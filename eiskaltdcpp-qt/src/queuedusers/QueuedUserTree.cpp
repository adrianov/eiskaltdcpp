/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "queuedusers/QueuedUserTree.h"
#include "NaturalCompareQt.h"
#include "WulforUtil.h"

#include <algorithm>

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(int col, QList<QueuedUserItem*> &items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

private:
    typedef bool (*AttrComp)(const QueuedUserItem *l, const QueuedUserItem *r);
    AttrComp static getAttrComp(const int column) {
        static AttrComp attrs[2] = { AttrCmp<0>, NaturalAttrCmp<1> };
        return attrs[column];
    }
    template <int i>
    bool static AttrCmp(const QueuedUserItem *l, const QueuedUserItem *r) {
        return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int i>
    bool static NaturalAttrCmp(const QueuedUserItem *l, const QueuedUserItem *r) {
        return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <typename T>
    bool static Cmp(const T &l, const T &r);
};

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T &l, const T &r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T &l, const T &r) {
    return l > r;
}

} // namespace

QueuedUserTree::QueuedUserTree(const QList<QVariant> &headers) :
    rootItem(new QueuedUserItem(headers))
{
}

QueuedUserTree::~QueuedUserTree() {
    delete rootItem;
}

QueuedUserItem *QueuedUserTree::user(const QString &cid) const {
    return cids.value(cid);
}

QueuedUserItem *QueuedUserTree::makeItem(const QList<QVariant> &data, QueuedUserItem *parent,
                                         const QString &cid, const QString &file, const QString &hub) {
    QueuedUserItem *item = new QueuedUserItem(data, parent);
    item->cid = cid;
    item->file = file;
    item->hub = hub;
    return item;
}

QueuedUserItem *QueuedUserTree::addUser(const VarMap &map) {
    const QString cid = map.value(QLatin1String("CID")).toString();
    if (cid.isEmpty() || cids.contains(cid))
        return cids.value(cid);

    const QString file = map.value(QLatin1String("FILE")).toString();
    const QString hub = map.value(QLatin1String("HUB")).toString();
    QueuedUserItem *item = makeItem(
        QList<QVariant>() << WulforUtil::getInstance()->getNicks(cid) << QString(),
        rootItem, cid, file, hub);
    rootItem->appendChild(item);
    cids.insert(cid, item);
    return item;
}

QueuedUserItem *QueuedUserTree::addFile(QueuedUserItem *user, const VarMap &map) {
    if (!user)
        return nullptr;

    const QString cid = map.value(QLatin1String("CID")).toString();
    const QString file = map.value(QLatin1String("FILE")).toString();
    const QString hub = map.value(QLatin1String("HUB")).toString();
    QueuedUserItem *item = makeItem(QList<QVariant>() << QString() << file, user, cid, file, hub);
    user->appendChild(item);
    return item;
}

QueuedUserItem *QueuedUserTree::takeUser(const QString &cid) {
    auto it = cids.find(cid);
    if (it == cids.end())
        return nullptr;

    QueuedUserItem *item = it.value();
    if (!item)
        return nullptr;

    const int row = item->row();
    if (row < 0 || row >= rootItem->childCount())
        return nullptr;

    rootItem->childItems.removeAt(row);
    cids.erase(it);
    return item;
}

void QueuedUserTree::sortItems(QList<QueuedUserItem*> &items) const {
    if (items.empty() || sortColumn < 0 || sortColumn > 1)
        return;

    if (sortOrder == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>().sort(sortColumn, items);
    else
        Compare<Qt::DescendingOrder>().sort(sortColumn, items);

    for (QueuedUserItem *parent : items) {
        if (parent && parent->childCount())
            sortItems(parent->childItems);
    }
}

void QueuedUserTree::sort(int column, Qt::SortOrder order) {
    if (column >= 0 && column <= 1) {
        sortColumn = column;
        sortOrder = order;
    }
    if (!rootItem || rootItem->childItems.empty())
        return;
    sortItems(rootItem->childItems);
}
