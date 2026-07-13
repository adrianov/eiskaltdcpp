/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "DownloadQueueModelSort.h"
#include "NaturalCompareQt.h"

#include <set>

namespace {

template <Qt::SortOrder order>
struct Compare {
    typedef bool (*AttrComp)(const DownloadQueueItem * l, const DownloadQueueItem * r);

    void static sort(unsigned column, QList<DownloadQueueItem*>& items) {
        if (column > COLUMN_DOWNLOADQUEUE_TTH)
            return;

        std::stable_sort(items.begin(), items.end(), attrs[column]);
    }

    private:
        template <int i>
        bool static AttrCmp(const DownloadQueueItem * l, const DownloadQueueItem * r) {
            if (l->dir != r->dir)
                return (l->dir);
            return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int i>
        bool static NaturalAttrCmp(const DownloadQueueItem * l, const DownloadQueueItem * r) {
            if (l->dir != r->dir)
                return (l->dir);
            return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
        }
        template <int column>
        bool static NumCmp(const DownloadQueueItem * l, const DownloadQueueItem * r) {
            if (l->dir != r->dir)
                return (l->dir);
            return Cmp(l->data(column).toULongLong(), r->data(column).toULongLong());
       }
        template <typename T>
        bool static Cmp(const T& l, const T& r);

        static AttrComp attrs[11];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[11] = { Compare<order>::NaturalAttrCmp<COLUMN_DOWNLOADQUEUE_NAME>,
                                                                Compare<order>::AttrCmp<COLUMN_DOWNLOADQUEUE_DOWN>,
                                                                Compare<order>::NumCmp<COLUMN_DOWNLOADQUEUE_ESIZE>,
                                                                Compare<order>::NumCmp<COLUMN_DOWNLOADQUEUE_DOWN>,
                                                                Compare<order>::NumCmp<COLUMN_DOWNLOADQUEUE_PRIO>,
                                                                Compare<order>::AttrCmp<COLUMN_DOWNLOADQUEUE_USER>,
                                                                Compare<order>::NaturalAttrCmp<COLUMN_DOWNLOADQUEUE_PATH>,
                                                                Compare<order>::NumCmp<COLUMN_DOWNLOADQUEUE_ESIZE>,
                                                                Compare<order>::AttrCmp<COLUMN_DOWNLOADQUEUE_ERR>,
                                                                Compare<order>::AttrCmp<COLUMN_DOWNLOADQUEUE_ADDED>,
                                                                Compare<order>::AttrCmp<COLUMN_DOWNLOADQUEUE_TTH>
                                                              };

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

void sortRecursive(int column, Qt::SortOrder order, DownloadQueueItem *i){
    if (column == -1 || !i || !i->childCount())
        return;

    static Compare<Qt::AscendingOrder> acomp;
    static Compare<Qt::DescendingOrder> dcomp;

    if (order == Qt::AscendingOrder)
        acomp.sort(column, i->childItems);
    else if (order == Qt::DescendingOrder)
        dcomp.sort(column, i->childItems);

    for (const auto &ii : i->childItems)
        sortRecursive(column, order, ii);
}

} // namespace

void sortDownloadQueueItems(int column, Qt::SortOrder order, DownloadQueueItem *root) {
    sortRecursive(column, order, root);
}
