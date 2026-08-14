/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "filebrowser/ListFilterProxy.h"
#include "FileBrowserModel.h"

void ListFilterProxy::clearMap()
{
    rows_.clear();
    sourceToProxy_.clear();
    filtered_ = false;
}

void ListFilterProxy::setSourceModel(QAbstractItemModel *sourceModel)
{
    filter_.join(this);
    if (this->sourceModel())
        disconnect(this->sourceModel(), nullptr, this, nullptr);

    beginResetModel();
    QAbstractProxyModel::setSourceModel(sourceModel);
    clearMap();
    endResetModel();
    if (!sourceModel)
        return;

    // Join before the source frees rows (beginResetModel → aboutToBeReset → qDeleteAll).
    connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, [this]() {
        filter_.join(this);
    });
    connect(sourceModel, &QAbstractItemModel::modelReset, this, [this]() {
        beginResetModel();
        clearMap();
        endResetModel();
        if (filter_.isActive())
            scheduleFilter();
    });
    connect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged, this, [this]() {
        if (!filter_.isActive())
            emit layoutAboutToBeChanged();
    });
    connect(sourceModel, &QAbstractItemModel::layoutChanged, this, [this]() {
        // Sort reorders existing rows; do not reset (autosize hooks modelReset).
        filter_.cancel();
        if (filter_.isActive())
            scheduleFilter(false);
        else
            emit layoutChanged();
    });
}

void ListFilterProxy::sort(int column, Qt::SortOrder order)
{
    if (sourceModel())
        sourceModel()->sort(column, order);
}

QModelIndex ListFilterProxy::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel() || row < 0 || column < 0
            || row >= rowCount() || column >= columnCount())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex ListFilterProxy::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int ListFilterProxy::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel())
        return 0;
    return filtered_ ? rows_.size() : sourceModel()->rowCount();
}

int ListFilterProxy::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel())
        return 0;
    return sourceModel()->columnCount();
}

QModelIndex ListFilterProxy::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || !sourceModel())
        return QModelIndex();
    const int sourceRow = filtered_ ? rows_.value(proxyIndex.row(), -1) : proxyIndex.row();
    if (sourceRow < 0)
        return QModelIndex();
    return sourceModel()->index(sourceRow, proxyIndex.column());
}

QModelIndex ListFilterProxy::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.parent().isValid() || !sourceModel())
        return QModelIndex();
    if (!filtered_)
        return index(sourceIndex.row(), sourceIndex.column());
    const auto it = sourceToProxy_.constFind(sourceIndex.row());
    if (it == sourceToProxy_.cend())
        return QModelIndex();
    return index(it.value(), sourceIndex.column());
}

void ListFilterProxy::setIdentity()
{
    if (!filtered_)
        return;
    beginResetModel();
    clearMap();
    endResetModel();
}

void ListFilterProxy::adoptRows(QVector<int> rows)
{
    rows_ = std::move(rows);
    sourceToProxy_.clear();
    sourceToProxy_.reserve(rows_.size());
    for (int i = 0; i < rows_.size(); ++i)
        sourceToProxy_.insert(rows_.at(i), i);
    filtered_ = true;
}

void ListFilterProxy::setRows(QVector<int> rows, bool reset)
{
    if (!reset && filtered_ && rows.size() == rows_.size()) {
        emit layoutAboutToBeChanged();
        adoptRows(std::move(rows));
        emit layoutChanged();
        return;
    }
    beginResetModel();
    adoptRows(std::move(rows));
    endResetModel();
}

void ListFilterProxy::scheduleFilter(bool reset)
{
    if (!filter_.isActive()) {
        setIdentity();
        return;
    }

    const FileBrowserModel *fb = qobject_cast<const FileBrowserModel*>(sourceModel());
    FileBrowserItem *root = fb ? const_cast<FileBrowserModel*>(fb)->getRootElem() : nullptr;
    if (!root)
        return;

    if (filter_.shouldAsync(root->childCount())) {
        filter_.scanAsync(root, pathPrefix_, this, [this, reset](QVector<int> rows) {
            setRows(std::move(rows), reset);
        });
        return;
    }

    QVector<int> rows;
    rows.reserve(root->childCount());
    for (int i = 0; i < root->childCount(); ++i) {
        if (filter_.acceptItem(root->child(i), pathPrefix_))
            rows.append(i);
    }
    setRows(std::move(rows), reset);
}

void ListFilterProxy::applyFilters(const FilterMatch &match, const QString &pathPrefix)
{
    const bool critChanged = filter_.set(match);
    const bool pathChanged = (pathPrefix_ != pathPrefix);
    pathPrefix_ = pathPrefix;
    if (!critChanged && !pathChanged)
        return;
    if (pathChanged && !critChanged)
        filter_.cancel();
    scheduleFilter();
}
