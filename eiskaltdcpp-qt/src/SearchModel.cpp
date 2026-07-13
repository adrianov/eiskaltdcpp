/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QList>
#include <QStringList>

#include "SearchModel.h"
#include "SearchModelSort.h"
#include "SearchFrame.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"

using namespace dcpp;

SearchModel::SearchModel(QObject *parent):
        QAbstractItemModel(parent),
        filterRole(SearchFrame::None),
        sortColumn(COLUMN_SF_ESIZE),
        sortOrder(Qt::DescendingOrder)
{
    QList<QVariant> rootData;
    rootData << tr("Count") << tr("File") << tr("Ext") << tr("Size")
             << tr("Exact size") << QString("TTH")   << tr("Path") << tr("Nick")
             << tr("Free slots") << tr("Total slots")
             << tr("IP") << tr("Hub") << tr("Host");

    rootItem = new SearchItem(rootData);

    sortColumn = -1;
}

SearchModel::~SearchModel()
{
    delete rootItem;
}

void SearchModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (sortColumn < 0 || sortColumn > columnCount()-1)
        return;

    emit layoutAboutToBeChanged();
    const QModelIndexList oldIndexes = persistentIndexList();

    try {
        sortSearchItems(column, order, rootItem->childItems);
    }
    catch (SearchListException &) {
        sortColumn = COLUMN_SF_FILENAME;
        sortSearchItems(COLUMN_SF_FILENAME, order, rootItem->childItems);
    }

    QModelIndexList newIndexes;
    newIndexes.reserve(oldIndexes.size());
    for (const QModelIndex &idx : oldIndexes) {
        auto *item = static_cast<SearchItem*>(idx.internalPointer());
        if (!item || item == rootItem)
            newIndexes << QModelIndex();
        else
            newIndexes << createIndex(item->row(), idx.column(), item);
    }
    changePersistentIndexList(oldIndexes, newIndexes);
    emit layoutChanged();
}

void SearchModel::flushDeferredSort() {
    if (!countSortPending)
        return;
    countSortPending = false;
    if (sortColumn == COLUMN_SF_COUNT)
        sort(sortColumn, sortOrder);
}

int SearchModel::getSortColumn() const {
    return sortColumn;
}

void SearchModel::setSortColumn(int c) {
    sortColumn = c;
}

Qt::SortOrder SearchModel::getSortOrder() const {
    return sortOrder;
}

void SearchModel::setSortOrder(Qt::SortOrder o) {
    sortOrder = o;
}
