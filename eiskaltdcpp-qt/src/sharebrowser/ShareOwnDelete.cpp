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

#include "sharebrowser/ShareOwnDelete.h"
#include "sharebrowser/ShareOwnFileDelete.h"
#include "FileBrowserModel.h"
#include "WulforUtil.h"

#include "dcpp/ShareManager.h"

#include <QDir>

using namespace dcpp;

ShareOwnDelete::ShareOwnDelete(DirectoryListing &listing)
    : listing(listing)
{
}

FileBrowserItem *ShareOwnDelete::itemAt(const QModelIndex &index)
{
    return reinterpret_cast<FileBrowserItem*>(index.internalPointer());
}

bool ShareOwnDelete::underDir(Directory *anc, Directory *dir)
{
    for (; dir; dir = dir->getParent()) {
        if (dir == anc)
            return true;
    }
    return false;
}

bool ShareOwnDelete::isNested(Directory *dir) const
{
    Directory *root = listing.getRoot();
    return dir && root && dir != root && dir->getParent() && dir->getParent() != root
            && !dynamic_cast<DirectoryListing::AdlDirectory*>(dir);
}

QVector<ShareOwnDelete::Directory*> ShareOwnDelete::topmostDirs(const QSet<Directory*> &dirs) const
{
    QVector<Directory*> top;
    for (Directory *dir : dirs) {
        bool nested = false;
        for (Directory *other : dirs) {
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

void ShareOwnDelete::removeDiskDir(const string &realPath)
{
    if (!ShareManager::getInstance()->isNestedShareDir(realPath))
        return;

    QString qpath = _q(realPath);
    while (qpath.endsWith(QLatin1Char('/')) || qpath.endsWith(QLatin1Char('\\')))
        qpath.chop(1);
    if (!qpath.isEmpty())
        QDir(qpath).removeRecursively();
}

int ShareOwnDelete::otherCopyCount(const QModelIndexList &list) const
{
    return ShareOwnFileDelete(listing).otherCopyCount(list);
}

ShareOwnDelete::Directory *ShareOwnDelete::nestedDir(FileBrowserItem *item) const
{
    if (!item)
        return nullptr;
    if (item->dir)
        return isNested(item->dir) ? item->dir : nullptr;
    if (item->file)
        return isNested(item->file->getParent()) ? item->file->getParent() : nullptr;
    return nullptr;
}

ShareOwnDelete::Directory *ShareOwnDelete::deleteFiles(const QModelIndexList &list)
{
    return ShareOwnFileDelete(listing).deleteFiles(list);
}

ShareOwnDelete::Directory *ShareOwnDelete::deleteOtherCopies(const QModelIndexList &list)
{
    return ShareOwnFileDelete(listing).deleteOtherCopies(list);
}

ShareOwnDelete::DirResult ShareOwnDelete::deleteDirs(const QModelIndexList &list)
{
    QSet<Directory*> dirs;
    for (const auto &index : list) {
        if (Directory *dir = nestedDir(itemAt(index)))
            dirs.insert(dir);
    }

    DirResult result;
    for (Directory *dir : topmostDirs(dirs)) {
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

        result.viewParent = dir->getParent();
        for (const auto &realPath : paths) {
            try {
                ShareManager::getInstance()->removeDir(realPath);
            } catch (const std::exception&) {}
            removeDiskDir(realPath);
        }
        if (!result.viewParent)
            continue;
        result.viewParent->directories.erase(dir);
        delete dir;
        result.removedDir = true;
    }
    return result;
}
