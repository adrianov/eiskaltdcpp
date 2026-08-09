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

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

#include <QString>
#include <QStringList>
#include <vector>

class FileBrowserItem;

/** Copyable criteria + match rules for share-list rows and directory subtrees. */
class FilterMatch
{
public:
    struct Term {
        QString value;
        bool exclude = false;
    };

    std::vector<Term> terms;
    qulonglong sizeLimit = 0;
    int sizeMode = 0;
    bool dirsOnly = false;
    bool filesOnly = false;
    QStringList extFilter;

    bool needTth() const;
    bool matchesText(const QString &haystack) const;
    bool dirPasses(const QString &path, qulonglong size) const;
    bool acceptFile(const QString &name, const QString &path, const QString &tth,
                    qulonglong size) const;
    bool filePasses(const QString &filePath, qulonglong size, const QString &tth) const;
    bool acceptItem(FileBrowserItem *item, const QString &pathPrefix) const;
    bool subtreeHasMatch(dcpp::DirectoryListing::Directory *dir, const QString &path) const;
    bool subtreeHasVisibleDir(dcpp::DirectoryListing::Directory *dir, const QString &path) const;

    bool isActive() const;
    void setTerms(const QStringList &raw);
};
