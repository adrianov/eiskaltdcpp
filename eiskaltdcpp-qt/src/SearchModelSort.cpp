/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModelSort.h"
#include "NaturalCompareQt.h"

#include <algorithm>

namespace {

template <Qt::SortOrder order>
struct Compare {
    typedef bool (*AttrComp)(const SearchItem * l, const SearchItem * r);

    void static sort(int column, QList<SearchItem*>& items) {
        if (column < 0 || column > static_cast<int>(COLUMN_SF_HOST))
            return;

        std::stable_sort(items.begin(), items.end(), attrs[column]);
    }

    QList<SearchItem*>::iterator static insertSorted(int column, QList<SearchItem*>& items, SearchItem* item) {
        if (column < 0 || column > static_cast<int>(COLUMN_SF_HOST))
            return items.end();

        return std::lower_bound(items.begin(),
                           items.end(),
                           item,
                           attrs[column]
                          );
    }

    private:
        template <int i>
        bool static AttrCmp(const SearchItem * l, const SearchItem * r) {
            return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        bool static NaturalAttrCmp(const SearchItem * l, const SearchItem * r) {
            return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <typename T, T (SearchItem::*attr)>
        bool static AttrCmp(const SearchItem * l, const SearchItem * r) {
            return Cmp(l->*attr, r->*attr);
        }
        template <int i>
        bool static NumCmp(const SearchItem * l, const SearchItem * r) {
            return Cmp(l->data(i).toULongLong(), r->data(i).toULongLong());
        }
        template <typename T>
        bool static Cmp(const T& l, const T& r);

        static AttrComp attrs[13];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[13] = { NumCmp<COLUMN_SF_COUNT>,
                                                                NaturalAttrCmp<COLUMN_SF_FILENAME>,
                                                                AttrCmp<COLUMN_SF_EXTENSION>,
                                                                NumCmp<COLUMN_SF_ESIZE>,
                                                                NumCmp<COLUMN_SF_ESIZE>,
                                                                AttrCmp<COLUMN_SF_TTH>,
                                                                NaturalAttrCmp<COLUMN_SF_PATH>,
                                                                AttrCmp<COLUMN_SF_NICK>,
                                                                NumCmp<COLUMN_SF_FREESLOTS>,
                                                                NumCmp<COLUMN_SF_ALLSLOTS>,
                                                                AttrCmp<COLUMN_SF_IP>,
                                                                AttrCmp<COLUMN_SF_HUB>,
                                                                AttrCmp<COLUMN_SF_HOST>
                                                                };

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

} //namespace

void sortSearchItems(int column, Qt::SortOrder order, QList<SearchItem*> &items) {
    static Compare<Qt::AscendingOrder>  acomp = Compare<Qt::AscendingOrder>();
    static Compare<Qt::DescendingOrder> dcomp = Compare<Qt::DescendingOrder>();

    if (order == Qt::AscendingOrder)
        acomp.sort(column, items);
    else if (order == Qt::DescendingOrder)
        dcomp.sort(column, items);
}

QList<SearchItem*>::iterator insertSortedSearchItem(int column, Qt::SortOrder order,
                                                    QList<SearchItem*> &items, SearchItem *item) {
    static Compare<Qt::AscendingOrder>  acomp = Compare<Qt::AscendingOrder>();
    static Compare<Qt::DescendingOrder> dcomp = Compare<Qt::DescendingOrder>();

    if (order == Qt::AscendingOrder)
        return acomp.insertSorted(column, items, item);

    return dcomp.insertSorted(column, items, item);
}
