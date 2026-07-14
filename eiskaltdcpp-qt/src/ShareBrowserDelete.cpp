/***************************************************************************
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

} // namespace

void ShareBrowser::deleteOwnItems(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe())
        return;

    QVector<FileBrowserItem*> files;
    QVector<FileBrowserItem*> dirs;

    for (const auto &index : list) {
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(index.internalPointer());
        if (!item)
            continue;
        if (item->file)
            files.push_back(item);
        else if (item->dir && item->dir != listing.getRoot()
                 && !dynamic_cast<DirectoryListing::AdlDirectory*>(item->dir))
            dirs.push_back(item);
    }

    // Only topmost selected dirs (avoid use-after-free when parent+child are selected).
    QVector<FileBrowserItem*> topDirs;
    for (FileBrowserItem *item : dirs) {
        bool nested = false;
        for (FileBrowserItem *other : dirs) {
            if (other != item && underDir(other->dir, item->dir)) {
                nested = true;
                break;
            }
        }
        if (!nested)
            topDirs.push_back(item);
    }

    DirectoryListing::Directory *viewParent = nullptr;
    bool removedDir = false;

    for (FileBrowserItem *item : files) {
        DirectoryListing::File *file = item->file;
        if (!file)
            continue;

        bool covered = false;
        for (FileBrowserItem *d : topDirs) {
            if (underDir(d->dir, file->getParent())) {
                covered = true;
                break;
            }
        }
        if (covered)
            continue;

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

    for (FileBrowserItem *item : topDirs) {
        DirectoryListing::Directory *dir = item->dir;
        if (!dir)
            continue;

        const StringList paths = listing.getLocalPaths(dir);
        if (paths.empty())
            continue;

        for (const auto &realPath : paths) {
            try {
                ShareManager::getInstance()->removeDir(realPath);
            } catch (const std::exception&) {}

            QString qpath = _q(realPath);
            while (qpath.endsWith(QLatin1Char('/')) || qpath.endsWith(QLatin1Char('\\')))
                qpath.chop(1);
            QDir(qpath).removeRecursively();
        }

        viewParent = dir->getParent();
        if (!viewParent)
            continue;

        viewParent->directories.erase(dir);
        item->dir = nullptr;
        delete dir;
        removedDir = true;
    }

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
            const QModelIndex ix = treeMapFromSource(tree_model->createIndexForItem(ti));
            treeView_LPANE->selectionModel()->select(ix,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            return; // selection slot refreshes the right pane
        }
    }

    changeRoot(viewParent);
}
