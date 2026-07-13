/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "FavoriteHubModel.h"
#include "FavoriteHubModelSort.h"

#include <QList>
#include <QStringList>

FavoriteHubModel::FavoriteHubModel(QObject *parent)
    : QAbstractItemModel(parent), sortColumn(-1), sortOrder(Qt::AscendingOrder)
{
    QList<QVariant> rootData;
    rootData << tr("Autoconnect") << tr("Name") << tr("Description")
             << tr("Address") << tr("Nick") << tr("Password") << tr("User description")
             << tr("Remote encoding");

    rootItem = new FavoriteHubItem(rootData, nullptr);
}

FavoriteHubModel::~FavoriteHubModel()
{
    delete rootItem;
}

int FavoriteHubModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FavoriteHubItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant FavoriteHubModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() > columnCount(QModelIndex()))
        return QVariant();

    FavoriteHubItem *item = static_cast<FavoriteHubItem*>(index.internalPointer());

    switch(role) {
        case Qt::DecorationRole: // icon
            break;
        case Qt::DisplayRole:
            if (index.column() == COLUMN_HUB_AUTOCONNECT)
                break;
            else if (index.column() != COLUMN_HUB_PASSWORD)
                return item->data(index.column());
            else
                return QString("******");

            break;
        case Qt::TextAlignmentRole:
            break;
        case Qt::ForegroundRole:
            break;
        case Qt::ToolTipRole:
            break;
        case Qt::CheckStateRole:
            if (index.column() == COLUMN_HUB_AUTOCONNECT)
                return item->data(COLUMN_HUB_AUTOCONNECT);
            break;
    }

    return QVariant();
}

Qt::ItemFlags FavoriteHubModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    if (index.isValid())
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    else
        flags |= Qt::ItemIsDragEnabled;

    if (index.column() == COLUMN_HUB_AUTOCONNECT)
        flags |= Qt::ItemIsUserCheckable;
    else if (index.column() == COLUMN_HUB_PASSWORD || index.column() == COLUMN_HUB_ENCODING)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

QVariant FavoriteHubModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex FavoriteHubModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FavoriteHubItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FavoriteHubItem*>(parent.internalPointer());

    FavoriteHubItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex FavoriteHubModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    FavoriteHubItem *childItem = static_cast<FavoriteHubItem*>(index.internalPointer());
    FavoriteHubItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int FavoriteHubModel::rowCount(const QModelIndex &parent) const
{
    FavoriteHubItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FavoriteHubItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void FavoriteHubModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (!rootItem || rootItem->childItems.empty() || column == -1)
        return;

    emit layoutAboutToBeChanged();
    sortFavoriteHubItems(column, order, rootItem->childItems);
    emit layoutChanged();
}
