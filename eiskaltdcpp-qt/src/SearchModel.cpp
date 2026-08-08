/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtWidgets>

#include <QList>
#include <QStringList>

#include "SearchModel.h"
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
             << tr("IP") << tr("Hub") << tr("Host")
             << tr("Bitrate") << tr("Resolution") << tr("Video") << tr("Audio");

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

    const auto sortTree = [this](int col) {
        rootItem->sortChildren(col, sortOrder);
        for (SearchItem *group : rootItem->children()) {
            if (group->childCount() > 1)
                group->sortChildren(col, sortOrder);
        }
    };

    try {
        sortTree(column);
    }
    catch (SearchListException &) {
        sortColumn = COLUMN_SF_FILENAME;
        sortTree(COLUMN_SF_FILENAME);
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
