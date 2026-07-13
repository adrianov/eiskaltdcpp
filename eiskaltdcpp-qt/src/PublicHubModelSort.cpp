/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubModelSort.h"
#include "NaturalCompareQt.h"

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(int col, QList<PublicHubItem*>& items) {
        std::stable_sort(items.begin(), items.end(), getAttrComp(col));
    }

private:
    typedef bool (*AttrComp)(const PublicHubItem * l, const PublicHubItem * r);
    AttrComp static getAttrComp(int column) {
        switch (column){
            case COLUMN_PHUB_MAXHUBS:
                 return NumCmp<COLUMN_PHUB_MAXHUBS>;
            case COLUMN_PHUB_MAXUSERS:
                 return NumCmp<COLUMN_PHUB_MAXUSERS>;
            case COLUMN_PHUB_MINSHARE:
                 return NumCmp<COLUMN_PHUB_MINSHARE>;
            case COLUMN_PHUB_MINSLOTS:
                 return NumCmp<COLUMN_PHUB_MINSLOTS>;
            case COLUMN_PHUB_REL:
                 return DblCmp<COLUMN_PHUB_REL>;
            case COLUMN_PHUB_SHARED:
                 return NumCmp<COLUMN_PHUB_SHARED>;
            case COLUMN_PHUB_ADDRESS:
                 return AttrCmp<COLUMN_PHUB_ADDRESS>;
            case COLUMN_PHUB_COUNTRY:
                 return AttrCmp<COLUMN_PHUB_COUNTRY>;
            case COLUMN_PHUB_DESC:
                 return AttrCmp<COLUMN_PHUB_DESC>;
            case COLUMN_PHUB_NAME:
                 return NaturalAttrCmp<COLUMN_PHUB_NAME>;
            case COLUMN_PHUB_RATING:
                 return AttrCmp<COLUMN_PHUB_RATING>;
            default:
                 return NumCmp<COLUMN_PHUB_USERS>;
        }
    }
    template <int i>
    bool static AttrCmp(const PublicHubItem * l, const PublicHubItem * r) {
        return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int i>
    bool static NaturalAttrCmp(const PublicHubItem * l, const PublicHubItem * r) {
        return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int column>
    bool static NumCmp(const PublicHubItem * l, const PublicHubItem * r) {
        return Cmp(l->data(column).toULongLong(), r->data(column).toULongLong());
    }
    template <int column>
    bool static DblCmp(const PublicHubItem * l, const PublicHubItem * r) {
        return Cmp(l->data(column).toDouble(), r->data(column).toDouble());
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

void sortPublicHubItems(int column, Qt::SortOrder order, QList<PublicHubItem*> &items) {
    if (items.empty() || column < 0)
        return;

    if (order == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>().sort(column, items);
    else if (order == Qt::DescendingOrder)
        Compare<Qt::DescendingOrder>().sort(column, items);
}
