/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "QueuedUsersSort.h"
#include "NaturalCompareQt.h"

#include <algorithm>

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(int col, QList<QueuedUserItem*>& items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

private:
    typedef bool (*AttrComp)(const QueuedUserItem * l, const QueuedUserItem * r);
    AttrComp static getAttrComp(const int column) {
        static AttrComp attrs[2] = { AttrCmp<0>, NaturalAttrCmp<1> };
        return attrs[column];
    }
    template <int i>
    bool static AttrCmp(const QueuedUserItem * l, const QueuedUserItem * r) {
        return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int i>
    bool static NaturalAttrCmp(const QueuedUserItem * l, const QueuedUserItem * r) {
        return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <typename T>
    bool static Cmp(const T& l, const T& r);
};

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

} // namespace

void sortQueuedUserItems(int column, Qt::SortOrder order, QList<QueuedUserItem*> &items) {
    if (items.empty() || column < 0 || column > 1)
        return;

    if (order == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>().sort(column, items);
    else if (order == Qt::DescendingOrder)
        Compare<Qt::DescendingOrder>().sort(column, items);

    for (QueuedUserItem *parent : items) {
        if (parent && parent->childCount())
            sortQueuedUserItems(column, order, parent->childItems);
    }
}
