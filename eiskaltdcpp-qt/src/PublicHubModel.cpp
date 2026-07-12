/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PublicHubModel.h"
#include "PublicHubModelSort.h"

#include <QList>
#include <QStringList>

void PublicHubProxyModel::sort(int column, Qt::SortOrder order){
    if (sourceModel())
        sourceModel()->sort(column, order);
}

PublicHubModel::PublicHubModel(QObject *parent)
    : QAbstractItemModel(parent), sortColumn(-1), sortOrder(Qt::AscendingOrder)
{
    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Description") << tr("Users") << tr("Address")
             << tr("Country") << tr("Shared") << tr("Min share") << tr("Min slots")
             << tr("Max hubs") << tr("Max users") << tr("Reliability") << tr("Rating");

    rootItem = new PublicHubItem(rootData, nullptr);
}

PublicHubModel::~PublicHubModel()
{
    delete rootItem;
}

int PublicHubModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<PublicHubItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

Qt::ItemFlags PublicHubModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant PublicHubModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex PublicHubModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    PublicHubItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PublicHubItem*>(parent.internalPointer());

    PublicHubItem *childItem = parentItem->child(row);

    if (childItem && rootItem->childItems.contains(childItem))
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex PublicHubModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int PublicHubModel::rowCount(const QModelIndex &parent) const
{
    PublicHubItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PublicHubItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void PublicHubModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (!rootItem || rootItem->childItems.empty() || column == -1)
        return;

    emit layoutAboutToBeChanged();
    sortPublicHubItems(column, order, rootItem->childItems);
    emit layoutChanged();
}

void PublicHubModel::clearModel(){
    emit layoutAboutToBeChanged();

    beginRemoveRows(QModelIndex(), 0, rootItem->childCount()-1);
    qDeleteAll(rootItem->childItems);
    rootItem->childItems.clear();
    endRemoveRows();

    emit layoutChanged();
}

void PublicHubModel::addResult(const QList<QVariant> &data, dcpp::HubEntry *entry){

    PublicHubItem *item = new PublicHubItem(data, rootItem);
    item->entry = entry;

    beginInsertRows(QModelIndex(), rootItem->childCount(), rootItem->childCount());
    rootItem->appendChild(item);
    endInsertRows();

    emit layoutChanged();
}

void PublicHubModel::refreshConnected(){
    if (!rootItem || rootItem->childItems.empty())
        return;

    const int last = rootItem->childCount() - 1;
    emit dataChanged(index(0, 0), index(last, columnCount() - 1));
}
