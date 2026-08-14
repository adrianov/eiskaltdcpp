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

#include "filebrowser/ListingMatch.h"

#include <QVector>
#include <atomic>
#include <functional>
#include <memory>

class FileBrowserItem;
class QObject;

/**
 * Owns filter criteria, generation tokens, and async list scans.
 * File/dir predicates live in FilterMatch; tree walks in ListingMatch.
 */
class ListFilter
{
public:
    static constexpr int asyncRows = 4000;

    ListFilter() = default;
    ~ListFilter() { ++(*gen_); }

    bool set(FilterMatch m);
    void cancel();
    /**
     * Cancel scans and wait until workers parented to owner exit.
     * No timeout: workers hold FileBrowserItem* and must finish before model reset.
     * cancel() is observed at least every 64 list rows / files in scanAsync and
     * ListingMatch subtree walks (dirs-only size filter may briefly walk totals).
     */
    void join(QObject *owner);

    bool isActive() const { return match_.filter.isActive(); }
    bool dirsOnly() const { return match_.filter.dirsOnly; }
    bool acceptItem(FileBrowserItem *item, const QString &pathPrefix,
                    const std::atomic<int> *gen = nullptr, int expect = 0) const {
        return match_.acceptItem(item, pathPrefix, gen, expect);
    }
    bool subtreeHasMatch(dcpp::DirectoryListing::Directory *dir, const QString &path,
                         const std::atomic<int> *gen = nullptr, int expect = 0) const {
        return match_.subtreeHasMatch(dir, path, gen, expect);
    }
    bool subtreeHasVisibleDir(dcpp::DirectoryListing::Directory *dir, const QString &path,
                              const std::atomic<int> *gen = nullptr, int expect = 0) const {
        return match_.subtreeHasVisibleDir(dir, path, gen, expect);
    }

    bool shouldAsync(int rowCount) const { return rowCount >= asyncRows; }

    void scanAsync(FileBrowserItem *root, const QString &pathPrefix, QObject *receiver,
                   const std::function<void(QVector<int>)> &onDone);

private:
    ListingMatch match_;
    /** Shared so async workers can cancel-check after ListFilter is destroyed. */
    std::shared_ptr<std::atomic<int>> gen_ = std::make_shared<std::atomic<int>>(0);
};
