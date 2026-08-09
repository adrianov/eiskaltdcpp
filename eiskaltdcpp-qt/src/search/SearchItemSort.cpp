/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "search/SearchItem.h"
#include "NaturalCompareQt.h"

#include <algorithm>

namespace {

template <Qt::SortOrder order>
struct Compare {
    typedef bool (*AttrComp)(const SearchItem * l, const SearchItem * r);

    void static sort(int column, QList<SearchItem*>& items) {
        if (column < 0 || column > static_cast<int>(COLUMN_SF_LAST))
            return;

        std::stable_sort(items.begin(), items.end(), attrs[column]);
    }

    int static sortedRow(int column, const QList<SearchItem*>& items, SearchItem* item) {
        if (column < 0 || column > static_cast<int>(COLUMN_SF_LAST))
            return items.size();

        return static_cast<int>(std::lower_bound(items.begin(),
                           items.end(),
                           item,
                           attrs[column]
                          ) - items.begin());
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
        /** Natural Path, then File; negative if l precedes r. */
        int static pathThenFile(const SearchItem * l, const SearchItem * r) {
            const int byPath = compareNaturalQ(l->data(COLUMN_SF_PATH).toString(),
                                               r->data(COLUMN_SF_PATH).toString());
            if (byPath != 0)
                return byPath;
            return compareNaturalQ(l->data(COLUMN_SF_FILENAME).toString(),
                                   r->data(COLUMN_SF_FILENAME).toString());
        }
        /** Count first (follows header order); Path then File break ties (A→Z). */
        bool static CountCmp(const SearchItem * l, const SearchItem * r) {
            const auto lc = l->data(COLUMN_SF_COUNT).toULongLong();
            const auto rc = r->data(COLUMN_SF_COUNT).toULongLong();
            if (lc != rc)
                return Cmp(lc, rc);
            return pathThenFile(l, r) < 0;
        }
        /** Path first (follows header order); File breaks ties (A→Z). */
        bool static PathCmp(const SearchItem * l, const SearchItem * r) {
            const int byPath = compareNaturalQ(l->data(COLUMN_SF_PATH).toString(),
                                               r->data(COLUMN_SF_PATH).toString());
            if (byPath != 0)
                return Cmp(byPath, 0);
            return compareNaturalQ(l->data(COLUMN_SF_FILENAME).toString(),
                                   r->data(COLUMN_SF_FILENAME).toString()) < 0;
        }
        template <typename T>
        bool static Cmp(const T& l, const T& r);

        static AttrComp attrs[17];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[17] = { CountCmp,
                                                                NaturalAttrCmp<COLUMN_SF_FILENAME>,
                                                                AttrCmp<COLUMN_SF_EXTENSION>,
                                                                NumCmp<COLUMN_SF_ESIZE>,
                                                                PathCmp,
                                                                NumCmp<COLUMN_SF_ESIZE>,
                                                                AttrCmp<COLUMN_SF_TTH>,
                                                                AttrCmp<COLUMN_SF_NICK>,
                                                                NumCmp<COLUMN_SF_FREESLOTS>,
                                                                NumCmp<COLUMN_SF_ALLSLOTS>,
                                                                AttrCmp<COLUMN_SF_IP>,
                                                                AttrCmp<COLUMN_SF_HUB>,
                                                                AttrCmp<COLUMN_SF_HOST>,
                                                                NumCmp<COLUMN_SF_BR>,
                                                                AttrCmp<COLUMN_SF_WH>,
                                                                AttrCmp<COLUMN_SF_MVIDEO>,
                                                                AttrCmp<COLUMN_SF_MAUDIO>
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

void SearchItem::sortChildren(int column, Qt::SortOrder order) {
    static Compare<Qt::AscendingOrder>  acomp = Compare<Qt::AscendingOrder>();
    static Compare<Qt::DescendingOrder> dcomp = Compare<Qt::DescendingOrder>();

    if (order == Qt::AscendingOrder)
        acomp.sort(column, childItems);
    else if (order == Qt::DescendingOrder)
        dcomp.sort(column, childItems);
}

int SearchItem::sortedInsertRow(int column, Qt::SortOrder order, SearchItem *item) const {
    static Compare<Qt::AscendingOrder>  acomp = Compare<Qt::AscendingOrder>();
    static Compare<Qt::DescendingOrder> dcomp = Compare<Qt::DescendingOrder>();

    if (order == Qt::AscendingOrder)
        return acomp.sortedRow(column, childItems, item);

    return dcomp.sortedRow(column, childItems, item);
}
