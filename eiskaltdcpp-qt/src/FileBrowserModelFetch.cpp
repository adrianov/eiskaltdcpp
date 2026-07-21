/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserModel.h"
#include "FileBrowserModelSort.h"
#include "WulforUtil.h"

using namespace dcpp;

bool FileBrowserModel::canFetchMore(const QModelIndex &parent) const{
    if (!listing)
        return false;

    FileBrowserItem *item = rootItem;
    if (parent.isValid()) {
        item = static_cast<FileBrowserItem*>(parent.internalPointer());
        if (!item)
            return false;
    }

    return (item->dir && (item->dir->directories.size() != static_cast<size_t>(item->childCount())));
}

void FileBrowserModel::fetchBranch(const QModelIndex &parent, dcpp::DirectoryListing::Directory *dir){
    if (!dir)
        return;

    FileBrowserItem *root = rootItem;
    if (parent.isValid()) {
        root = static_cast<FileBrowserItem*>(parent.internalPointer());
        if (!root)
            return;
    }
    FileBrowserItem *item;
    quint64 size = 0;
    QList<QVariant> data;

    size = dir->getTotalSize(true);

    data << _q(dir->getName())
         << WulforUtil::formatBytes(size)
         << size
         << "";

    item = new FileBrowserItem(data, root);
    item->dir = dir;

    beginInsertRows(parent, root->childCount(), root->childCount());
    {
        root->appendChild(item);
    }
    endInsertRows();
}

bool FileBrowserModel::hasChildren(const QModelIndex &parent) const{
    if (!parent.isValid())
        return true;

    FileBrowserItem *item = static_cast<FileBrowserItem*>(parent.internalPointer());
    return (item && item->dir && !item->dir->directories.empty());
}

void FileBrowserModel::fetchMore(const QModelIndex &parent){
    if (!listing)
        return;

    if (!parent.isValid()) {
        fetchBranch(parent, listing->getRoot());
        return;
    }

    FileBrowserItem *item = static_cast<FileBrowserItem*>(parent.internalPointer());
    // All-or-nothing: never append a second full directory set (would duplicate).
    if (!item || !item->dir || item->dir->directories.empty() || item->childCount())
        return;

    // Sort before notifying the view: a silent reorder after endInsertRows
    // desyncs QSortFilterProxyModel and crashes on scroll (flags/mapToSource).
    QList<FileBrowserItem*> built;
    built.reserve(static_cast<int>(item->dir->directories.size()));
    for (const auto &dir : item->dir->directories) {
        quint64 size = dir->getTotalSize(true);
        QList<QVariant> data;
        data << _q(dir->getName())
             << WulforUtil::formatBytes(size)
             << size
             << "";
        auto *child = new FileBrowserItem(data, item);
        child->dir = dir;
        built.append(child);
    }
    sortFileBrowserItemList(sortColumn, sortOrder, built);

    const QModelIndex i = createIndexForItem(item);
    beginInsertRows(i, 0, built.size() - 1);
    for (FileBrowserItem *child : built)
        item->appendChild(child);
    endInsertRows();
}
