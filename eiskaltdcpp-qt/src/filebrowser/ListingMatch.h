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

#include "filebrowser/FilterMatch.h"

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

#include <atomic>

class FileBrowserItem;

/** Applies FilterMatch to share-list rows and DirectoryListing subtrees. */
class ListingMatch
{
public:
    FilterMatch filter;

    bool acceptItem(FileBrowserItem *item, const QString &pathPrefix,
                    const std::atomic<int> *gen = nullptr, int expect = 0) const;
    bool subtreeHasMatch(dcpp::DirectoryListing::Directory *dir, const QString &path,
                         const std::atomic<int> *gen = nullptr, int expect = 0) const;
    bool subtreeHasVisibleDir(dcpp::DirectoryListing::Directory *dir, const QString &path,
                              const std::atomic<int> *gen = nullptr, int expect = 0) const;

private:
    bool filePasses(const QString &filePath, qulonglong size, const QString &tth) const;
};
