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

#pragma once

#include <QModelIndexList>
#include <QSet>
#include <QVector>

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

class FileBrowserItem;

/** Own-list nested folder removal; file/copy deletion is ShareOwnFileDelete. */
class ShareOwnDelete {
public:
    explicit ShareOwnDelete(dcpp::DirectoryListing &listing);

    int otherCopyCount(const QModelIndexList &list) const;
    dcpp::DirectoryListing::Directory *nestedDir(FileBrowserItem *item) const;
    dcpp::DirectoryListing::Directory *deleteFiles(const QModelIndexList &list);
    dcpp::DirectoryListing::Directory *deleteOtherCopies(const QModelIndexList &list);

    struct DirResult {
        dcpp::DirectoryListing::Directory *viewParent = nullptr;
        bool removedDir = false;
    };
    DirResult deleteDirs(const QModelIndexList &list);

private:
    using Directory = dcpp::DirectoryListing::Directory;

    static FileBrowserItem *itemAt(const QModelIndex &index);
    static bool underDir(Directory *anc, Directory *dir);
    bool isNested(Directory *dir) const;
    QVector<Directory*> topmostDirs(const QSet<Directory*> &dirs) const;
    static void removeDiskDir(const std::string &realPath);

    dcpp::DirectoryListing &listing;
};
