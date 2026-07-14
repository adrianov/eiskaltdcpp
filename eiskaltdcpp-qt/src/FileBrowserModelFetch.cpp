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

    FileBrowserItem *item = parent.isValid()? static_cast<FileBrowserItem*>(parent.internalPointer()) : rootItem;

    return (item->dir && (item->dir->directories.size() != static_cast<size_t>(item->childCount())));
}

void FileBrowserModel::fetchBranch(const QModelIndex &parent, dcpp::DirectoryListing::Directory *dir){
    if (!dir)
        return;

    FileBrowserItem *root = parent.isValid()? static_cast<FileBrowserItem*>(parent.internalPointer()) : rootItem;
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

    return (item->dir && !item->dir->directories.empty());
}

void FileBrowserModel::fetchMore(const QModelIndex &parent){
    if (!listing)
        return;

    if (!parent.isValid())
        fetchBranch(parent, listing->getRoot());
    else{
        FileBrowserItem *item = static_cast<FileBrowserItem*>(parent.internalPointer());
        if (!item || !item->dir)
            return;

        QModelIndex i = createIndexForItem(item);

        for (const auto &dir : item->dir->directories) //loading child directories
            fetchBranch(i, dir);

        sortFileBrowserItems(sortColumn, sortOrder, item);
    }
}
