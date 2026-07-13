/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "DownloadQueueModel.h"
#include "DownloadQueueModelPrivate.h"
#include "WulforUtil.h"

#include <QList>
#include <QStringList>

DownloadQueueModel::DownloadQueueModel(QObject *parent)
    : QAbstractItemModel(parent), d_ptr(new DownloadQueueModelPrivate())
{
    Q_D(DownloadQueueModel);

    d->total_files = 0;
    d->total_size  = 0;

    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Status") << tr("Size") << tr("Downloaded")
             << tr("Priority") << tr("User") << tr("Path") << tr("Exact size")
             << tr("Errors") << tr("Added") << QString("TTH");

    d->rootItem = new DownloadQueueItem(rootData, nullptr);

    d->sortColumn = COLUMN_DOWNLOADQUEUE_NAME;
    d->sortOrder = Qt::AscendingOrder;
}

DownloadQueueModel::~DownloadQueueModel()
{
    Q_D(DownloadQueueModel);

    if (d->rootItem)
        delete d->rootItem;

    delete d;
}

int DownloadQueueModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);

    if (parent.isValid())
        return static_cast<DownloadQueueItem*>(parent.internalPointer())->columnCount();
    else
        return d->rootItem->columnCount();
}

QVariant DownloadQueueModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Status") << tr("Size") << tr("Downloaded")
             << tr("Priority") << tr("User") << tr("Path") << tr("Exact size")
             << tr("Errors") << tr("Added") << QString("TTH");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootData.at(section);

    return QVariant();
}

QModelIndex DownloadQueueModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DownloadQueueItem *parentItem;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<DownloadQueueItem*>(parent.internalPointer());

    DownloadQueueItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex DownloadQueueModel::parent(const QModelIndex &index) const
{
    Q_D(const static DownloadQueueModel);

    if (!index.isValid())
        return QModelIndex();

    DownloadQueueItem *childItem = static_cast<DownloadQueueItem*>(index.internalPointer());
    DownloadQueueItem *parentItem = childItem->parent();

    if (parentItem == d->rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int DownloadQueueModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);
    DownloadQueueItem *parentItem;

    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<DownloadQueueItem*>(parent.internalPointer());

    return parentItem->childCount();
}

DownloadQueueItem *DownloadQueueModel::getRootElem() const{
    Q_D(const DownloadQueueModel);

    return d->rootItem;
}

int DownloadQueueModel::getSortColumn() const {
    Q_D(const DownloadQueueModel);

    return d->sortColumn;
}

void DownloadQueueModel::setSortColumn(int c) {
    Q_D(DownloadQueueModel);

    d->sortColumn = c;
}

Qt::SortOrder DownloadQueueModel::getSortOrder() const {
    Q_D(const DownloadQueueModel);

    return d->sortOrder;
}

void DownloadQueueModel::setSortOrder(Qt::SortOrder o) {
    Q_D(DownloadQueueModel);

    d->sortOrder = o;
}

QModelIndex DownloadQueueModel::createIndexForItem(DownloadQueueItem *item){
    Q_D(DownloadQueueModel);

    if (!d->rootItem || !item || item == d->rootItem)
        return QModelIndex();

    return createIndex(item->row(), 0, item);
}

