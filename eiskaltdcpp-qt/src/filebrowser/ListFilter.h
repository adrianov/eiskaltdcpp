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

#include <QVector>
#include <atomic>
#include <functional>
#include <memory>

class FileBrowserItem;
class QObject;

/**
 * Owns filter criteria, generation tokens, and async list scans.
 * Matching rules live in FilterMatch.
 */
class ListFilter
{
public:
    static constexpr int asyncRows = 4000;

    ListFilter() = default;
    ~ListFilter() { ++(*gen_); }

    bool set(const QStringList &terms, qulonglong size, int sizeMode,
             bool dirsOnly, bool filesOnly, const QStringList &exts);
    void cancel();

    bool isActive() const { return match_.isActive(); }
    bool dirsOnly() const { return match_.dirsOnly; }
    bool acceptItem(FileBrowserItem *item, const QString &pathPrefix) const {
        return match_.acceptItem(item, pathPrefix);
    }
    bool subtreeHasMatch(dcpp::DirectoryListing::Directory *dir, const QString &path) const {
        return match_.subtreeHasMatch(dir, path);
    }
    bool subtreeHasVisibleDir(dcpp::DirectoryListing::Directory *dir, const QString &path) const {
        return match_.subtreeHasVisibleDir(dir, path);
    }

    bool shouldAsync(int rowCount) const { return rowCount >= asyncRows; }

    void scanAsync(FileBrowserItem *root, const QString &pathPrefix, QObject *receiver,
                   const std::function<void(QVector<int>)> &onDone);

private:
    QStringList textTermsRaw_;
    FilterMatch match_;
    /** Shared so async workers can cancel-check after ListFilter is destroyed. */
    std::shared_ptr<std::atomic<int>> gen_ = std::make_shared<std::atomic<int>>(0);
};
