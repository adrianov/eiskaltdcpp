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

#include "sharebrowser/ShareOwnFileDelete.h"
#include "FileBrowserModel.h"
#include "WulforUtil.h"

#include "dcpp/ShareManager.h"
#include "dcpp/File.h"

#include <QSet>

using namespace dcpp;

ShareOwnFileDelete::ShareOwnFileDelete(DirectoryListing &listing)
    : listing(listing)
{
}

FileBrowserItem *ShareOwnFileDelete::itemAt(const QModelIndex &index)
{
    return reinterpret_cast<FileBrowserItem*>(index.internalPointer());
}

string ShareOwnFileDelete::joinPath(const string &dir, const string &name)
{
    if (dir.empty())
        return name;
    const char c = dir.back();
    if (c == PATH_SEPARATOR || c == '/' || c == '\\')
        return dir + name;
    return dir + PATH_SEPARATOR + name;
}

StringList ShareOwnFileDelete::diskPaths(File *file) const
{
    if (!file || file->getAdls())
        return StringList();

    StringList paths = listing.getLocalPaths(file);
    if (!paths.empty() || !file->getParent())
        return paths;

    for (const auto &dir : listing.getLocalPaths(file->getParent()))
        paths.push_back(joinPath(dir, file->getName()));
    return paths;
}

ShareOwnFileDelete::Directory *ShareOwnFileDelete::eraseFile(File *file)
{
    if (!file)
        return nullptr;

    Directory *parent = file->getParent();
    const StringList paths = diskPaths(file);

    StringList survivorPaths;
    for (File *copy : listing.findByTTH(file->getTTH())) {
        if (copy == file)
            continue;
        survivorPaths = diskPaths(copy);
        if (!survivorPaths.empty())
            break;
    }

    for (const auto &realPath : paths) {
        try {
            ShareManager::getInstance()->removeFile(realPath);
            dcpp::File::deleteFile(realPath);
        } catch (const std::exception&) {}
    }

    listing.removeTthFile(file);
    if (parent)
        parent->files.erase(file);
    delete file;

    if (!survivorPaths.empty())
        ShareManager::getInstance()->indexFile(survivorPaths.front());
    return parent;
}

QVector<ShareOwnFileDelete::File*> ShareOwnFileDelete::uniqueKeeps(const QModelIndexList &list) const
{
    QVector<File*> keeps;
    QSet<QString> tths;
    for (const auto &index : list) {
        FileBrowserItem *item = itemAt(index);
        if (!item || !item->file || item->file->getAdls())
            continue;
        const QString tth = _q(item->file->getTTH().toBase32());
        if (tths.contains(tth))
            return {};
        tths.insert(tth);
        keeps.push_back(item->file);
    }
    return keeps;
}

int ShareOwnFileDelete::otherCopyCount(const QModelIndexList &list) const
{
    int n = 0;
    for (File *keep : uniqueKeeps(list)) {
        const size_t copies = listing.tthCopyCount(keep->getTTH());
        if (copies > 1)
            n += static_cast<int>(copies - 1);
    }
    return n;
}

ShareOwnFileDelete::Directory *ShareOwnFileDelete::deleteFiles(const QModelIndexList &list)
{
    Directory *viewParent = nullptr;
    for (const auto &index : list) {
        FileBrowserItem *item = itemAt(index);
        if (!item || !item->file)
            continue;
        Directory *parent = eraseFile(item->file);
        if (!parent)
            continue;
        item->file = nullptr;
        viewParent = parent;
    }
    return viewParent;
}

ShareOwnFileDelete::Directory *ShareOwnFileDelete::deleteOtherCopies(const QModelIndexList &list)
{
    const auto keeps = uniqueKeeps(list);
    Directory *viewParent = nullptr;
    for (File *keep : keeps) {
        const auto copies = listing.findByTTH(keep->getTTH());
        if (copies.size() < 2)
            continue;

        viewParent = keep->getParent();
        for (File *file : copies) {
            if (file != keep)
                eraseFile(file);
        }
    }
    return viewParent;
}
