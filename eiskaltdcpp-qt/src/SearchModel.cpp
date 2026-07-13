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

#ifdef _DEBUG_QT_UI
#include <QtDebug>
#endif

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

int SearchModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SearchItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    SearchItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<SearchItem*>(parent.internalPointer());

    SearchItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex SearchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SearchItem *childItem = static_cast<SearchItem*>(index.internalPointer());
    SearchItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool SearchModel::hasChildren(const QModelIndex &parent) const{
    return (parent.isValid()? (static_cast<SearchItem*>(parent.internalPointer())->childCount() > 0) : true);
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    SearchItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<SearchItem*>(parent.internalPointer());

    return parentItem->childCount();
}

QModelIndex SearchModel::createIndexForItem(SearchItem *item){
    if (!(rootItem && item) || item == rootItem)
        return QModelIndex();

    // Column 0: rowCount()/hasChildren() only report children for column 0,
    // so branch expand icons require a column-0 parent index.
    return createIndex(item->row(), 0, item);
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

