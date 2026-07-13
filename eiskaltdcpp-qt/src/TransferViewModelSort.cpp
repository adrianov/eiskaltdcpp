/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewModel.h"
#include "NaturalCompareQt.h"

#include <algorithm>
#include <QList>
#include <QString>

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(unsigned col, QList<TransferViewItem*>& items) {
        std::stable_sort(items.begin(), items.end(), attrs[col]);
    }

private:
    typedef bool (*AttrComp)(const TransferViewItem * l, const TransferViewItem * r);

    template <int i>
    bool static AttrCmp(const TransferViewItem * l, const TransferViewItem * r) {
        return Cmp(QString::localeAwareCompare(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int i>
    bool static NaturalAttrCmp(const TransferViewItem * l, const TransferViewItem * r) {
        return Cmp(compareNaturalQ(l->data(i).toString(), r->data(i).toString()), 0);
    }
    template <int column>
    bool static NumCmp(const TransferViewItem * l, const TransferViewItem * r) {
        return Cmp(l->data(column).toULongLong(), r->data(column).toULongLong());
    }
    template <typename T>
    bool static Cmp(const T& l, const T& r);

    static AttrComp attrs[10];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[10] = {
    AttrCmp<COLUMN_TRANSFER_USERS>,
    NumCmp<COLUMN_TRANSFER_SPEED>,
    AttrCmp<COLUMN_TRANSFER_STATS>,
    AttrCmp<COLUMN_TRANSFER_FLAGS>,
    NumCmp<COLUMN_TRANSFER_SIZE>,
    NumCmp<COLUMN_TRANSFER_TLEFT>,
    NaturalAttrCmp<COLUMN_TRANSFER_FNAME>,
    AttrCmp<COLUMN_TRANSFER_HOST>,
    AttrCmp<COLUMN_TRANSFER_IP>,
    AttrCmp<COLUMN_TRANSFER_ENCRYPTION>
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

void TransferViewModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (!rootItem || rootItem->childItems.empty() || column < 0 || column > columnCount()-1)
        return;

    emit layoutAboutToBeChanged();

    if (order == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>().sort(column, rootItem->childItems);
    else if (order == Qt::DescendingOrder)
        Compare<Qt::DescendingOrder>().sort(column, rootItem->childItems);

    emit layoutChanged();
}

int TransferViewModel::getSortColumn() const {
    return sortColumn;
}

void TransferViewModel::setSortColumn(int c) {
    sortColumn = c;
}

Qt::SortOrder TransferViewModel::getSortOrder() const {
    return sortOrder;
}

void TransferViewModel::setSortOrder(Qt::SortOrder o) {
    sortOrder = o;
}
