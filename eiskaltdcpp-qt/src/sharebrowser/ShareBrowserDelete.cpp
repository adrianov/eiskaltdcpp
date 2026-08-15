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

#include "ShareBrowser.h"
#include "FileBrowserModel.h"

#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/File.h"

#include <QDir>
#include <QSet>
#include <QVector>

using namespace dcpp;

namespace {

bool underDir(DirectoryListing::Directory *anc, DirectoryListing::Directory *d)
{
    for (; d; d = d->getParent()) {
        if (d == anc)
            return true;
    }
    return false;
}

bool isNestedListingDir(DirectoryListing::Directory *dir, DirectoryListing::Directory *root)
{
    return dir && root && dir != root && dir->getParent() && dir->getParent() != root
            && !dynamic_cast<DirectoryListing::AdlDirectory*>(dir);
}

QVector<DirectoryListing::Directory*> topmostDirs(const QSet<DirectoryListing::Directory*> &dirs)
{
    QVector<DirectoryListing::Directory*> top;
    for (DirectoryListing::Directory *dir : dirs) {
        bool nested = false;
        for (DirectoryListing::Directory *other : dirs) {
            if (other != dir && underDir(other, dir)) {
                nested = true;
                break;
            }
        }
        if (!nested)
            top.push_back(dir);
    }
    return top;
}

void removeDiskDir(const string &realPath)
{
    if (!ShareManager::getInstance()->isNestedShareDir(realPath))
        return;

    QString qpath = _q(realPath);
    while (qpath.endsWith(QLatin1Char('/')) || qpath.endsWith(QLatin1Char('\\')))
        qpath.chop(1);
    if (qpath.isEmpty())
        return;
    QDir(qpath).removeRecursively();
}

} // namespace

DirectoryListing::Directory *ShareBrowser::nestedDeleteDir(FileBrowserItem *item)
{
    if (!item)
        return nullptr;
    DirectoryListing::Directory *root = listing.getRoot();
    if (item->dir)
        return isNestedListingDir(item->dir, root) ? item->dir : nullptr;
    if (item->file)
        return isNestedListingDir(item->file->getParent(), root) ? item->file->getParent() : nullptr;
    return nullptr;
}

void ShareBrowser::refreshAfterOwnDelete(DirectoryListing::Directory *viewParent, bool removedDir)
{
    if (removedDir) {
        pathHistory.clear();
        pathHistory_iter = pathHistory.end();
        tree_model->clear();
        tree_model->fetchMore(QModelIndex());
        const QModelIndex rootIdx = treeMapFromSource(tree_model->index(0, 0));
        if (rootIdx.isValid())
            treeView_LPANE->setExpanded(rootIdx, true);
    }

    if (!viewParent)
        return;

    if (removedDir) {
        FileBrowserItem *listRoot = tree_model->getRootElem()->child(0);
        QString remote = _q(listing.getPath(viewParent));
        while (remote.endsWith(QLatin1Char('\\')))
            remote.chop(1);

        FileBrowserItem *ti = remote.isEmpty() ? listRoot
            : tree_model->createRootForPath(remote, listRoot);
        if (ti) {
            if (flatMode) {
                goToFlatItem(ti);
                return;
            }
            const QModelIndex ix = treeMapFromSource(tree_model->createIndexForItem(ti));
            treeView_LPANE->selectionModel()->select(ix,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            return;
        }
    }

    reloadRightPane(flatMode ? currentDir() : viewParent);
    updateUpButton();
}

void ShareBrowser::deleteOwnItems(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;

    DirectoryListing::Directory *viewParent = nullptr;

    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (!item || !item->file)
            continue;

        DirectoryListing::File *file = item->file;
        const StringList paths = listing.getLocalPaths(file);
        if (paths.empty())
            continue;

        for (const auto &realPath : paths) {
            try {
                ShareManager::getInstance()->removeFile(realPath);
                File::deleteFile(realPath);
            } catch (const std::exception&) {}
        }

        viewParent = file->getParent();
        if (!viewParent)
            continue;

        viewParent->files.erase(file);
        item->file = nullptr;
        delete file;
    }

    refreshAfterOwnDelete(viewParent, false);
}

void ShareBrowser::deleteOwnWholeDir(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;

    QSet<DirectoryListing::Directory*> dirs;
    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (DirectoryListing::Directory *dir = nestedDeleteDir(item))
            dirs.insert(dir);
    }

    DirectoryListing::Directory *viewParent = nullptr;
    bool removedDir = false;

    for (DirectoryListing::Directory *dir : topmostDirs(dirs)) {
        if (!isNestedListingDir(dir, listing.getRoot()))
            continue;

        const StringList paths = listing.getLocalPaths(dir);
        if (paths.empty())
            continue;

        bool nestedDisk = true;
        for (const auto &realPath : paths) {
            if (!ShareManager::getInstance()->isNestedShareDir(realPath)) {
                nestedDisk = false;
                break;
            }
        }
        if (!nestedDisk)
            continue;

        viewParent = dir->getParent();
        for (const auto &realPath : paths) {
            try {
                ShareManager::getInstance()->removeDir(realPath);
            } catch (const std::exception&) {}
            removeDiskDir(realPath);
        }

        if (!viewParent)
            continue;

        viewParent->directories.erase(dir);
        delete dir;
        removedDir = true;
    }

    refreshAfterOwnDelete(viewParent, removedDir);
}
