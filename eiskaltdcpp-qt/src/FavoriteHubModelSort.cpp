/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "FavoriteHubModelSort.h"
#include "NaturalCompareQt.h"

#include <algorithm>

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(int col, QList<FavoriteHubItem*>& items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

private:
    typedef bool (*AttrComp)(const FavoriteHubItem * l, const FavoriteHubItem * r);
    AttrComp static getAttrComp(int column) {
        switch (column){
             case COLUMN_HUB_AUTOCONNECT:
                 return NumCmp<COLUMN_HUB_AUTOCONNECT>;
             case COLUMN_HUB_ADDRESS:
                 return AttrCmp<COLUMN_HUB_ADDRESS>;
             case COLUMN_HUB_DESC:
                 return AttrCmp<COLUMN_HUB_DESC>;
             case COLUMN_HUB_ENCODING:
                 return AttrCmp<COLUMN_HUB_ENCODING>;
             case COLUMN_HUB_NAME:
                 return NaturalAttrCmp<COLUMN_HUB_NAME>;
             case COLUMN_HUB_NICK:
                 return AttrCmp<COLUMN_HUB_NICK>;
             case COLUMN_HUB_PASSWORD:
                 return AttrCmp<COLUMN_HUB_PASSWORD>;
             default:
                 return AttrCmp<COLUMN_HUB_USERDESC>;
        }
    }
    template <int i>
    bool static AttrCmp(const FavoriteHubItem * l, const FavoriteHubItem * r) {
        return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int i>
    bool static NaturalAttrCmp(const FavoriteHubItem * l, const FavoriteHubItem * r) {
        return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int column>
    bool static NumCmp(const FavoriteHubItem * l, const FavoriteHubItem * r) {
        return Cmp(l->data(column).toULongLong(), r->data(column).toULongLong());
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

void sortFavoriteHubItems(int column, Qt::SortOrder order, QList<FavoriteHubItem*> &items) {
    if (items.empty() || column < 0)
        return;

    if (order == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>().sort(column, items);
    else if (order == Qt::DescendingOrder)
        Compare<Qt::DescendingOrder>().sort(column, items);
}
