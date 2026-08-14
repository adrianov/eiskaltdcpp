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

#include "filebrowser/FileBrowserFilterProxy.h"

FileBrowserFilterProxy::FileBrowserFilterProxy(QObject *parent):
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
}

void FileBrowserFilterProxy::sort(int column, Qt::SortOrder order) {
    Q_UNUSED(column);
    Q_UNUSED(order);
    // Tree is sorted in FileBrowserModel::fetchMore; forwarding crashes scrollTo via proxy.
}

QModelIndex FileBrowserFilterProxy::mapToSource(const QModelIndex &proxyIndex) const {
    if (!proxyIndex.isValid() || !proxyIndex.internalPointer())
        return QModelIndex();
    return QSortFilterProxyModel::mapToSource(proxyIndex);
}

QModelIndex FileBrowserFilterProxy::parent(const QModelIndex &child) const {
    if (!child.isValid() || !child.internalPointer())
        return QModelIndex();
    return QSortFilterProxyModel::parent(child);
}

void FileBrowserFilterProxy::applyFilters(const FilterMatch &match, const QString &pathPrefix) {
    Q_UNUSED(pathPrefix);
    if (!filter_.set(match))
        return;
    invalidateFilter();
}

bool FileBrowserFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (!filter_.isActive())
        return true;

    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    const QModelIndex nameIndex = model->index(sourceRow, COLUMN_FILEBROWSER_NAME, sourceParent);
    const FileBrowserItem *item = static_cast<FileBrowserItem*>(nameIndex.internalPointer());
    if (!item || !item->dir)
        return true;

    const FileBrowserModel *fbModel = qobject_cast<const FileBrowserModel*>(model);
    const QString path = fbModel ? fbModel->createRemotePath(const_cast<FileBrowserItem*>(item)) : QString();
    return filter_.dirsOnly()
            ? filter_.subtreeHasVisibleDir(item->dir, path)
            : filter_.subtreeHasMatch(item->dir, path);
}
