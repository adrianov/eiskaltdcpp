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
#include <QVector>

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

class FileBrowserItem;

/** Own-list file removal: selected files and extra copies of the same TTH. */
class ShareOwnFileDelete {
public:
    explicit ShareOwnFileDelete(dcpp::DirectoryListing &listing);

    int otherCopyCount(const QModelIndexList &list) const;
    dcpp::DirectoryListing::Directory *deleteFiles(const QModelIndexList &list);
    dcpp::DirectoryListing::Directory *deleteOtherCopies(const QModelIndexList &list);

private:
    using File = dcpp::DirectoryListing::File;
    using Directory = dcpp::DirectoryListing::Directory;

    static FileBrowserItem *itemAt(const QModelIndex &index);
    static std::string joinPath(const std::string &dir, const std::string &name);
    dcpp::StringList diskPaths(File *file) const;
    Directory *eraseFile(File *file);
    QVector<File*> uniqueKeeps(const QModelIndexList &list) const;

    dcpp::DirectoryListing &listing;
};
