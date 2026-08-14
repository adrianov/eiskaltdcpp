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

#include "filebrowser/ListingMatch.h"
#include "FileBrowserModel.h"
#include "WulforUtil.h"

#include "dcpp/SearchManager.h"

using namespace dcpp;

namespace {

bool stale(const std::atomic<int> *gen, int expect)
{
    return gen && gen->load() != expect;
}

int lastSep(const QString &path)
{
    return qMax(path.lastIndexOf(QLatin1Char('\\')), path.lastIndexOf(QLatin1Char('/')));
}

} // namespace

bool ListingMatch::filePasses(const QString &filePath, qulonglong size, const QString &tth) const
{
    const int slash = lastSep(filePath);
    const QString name = slash >= 0 ? filePath.mid(slash + 1) : filePath;
    const QString path = slash >= 0 ? filePath.left(slash) : QString();
    return filter.acceptFile(name, path, tth, size);
}

bool ListingMatch::subtreeHasMatch(DirectoryListing::Directory *dir, const QString &path,
                                   const std::atomic<int> *gen, int expect) const
{
    if (!dir || stale(gen, expect))
        return false;
    // Dir path can match on its own (empty dirs / tree parents); skip when ext-only.
    if (!filter.terms.empty() && filter.extFilter.isEmpty() && !filter.adultVideo
            && filter.matchesText(path))
        return true;
    const bool tth = filter.needTth();
    int n = 0;
    for (const auto &file : dir->files) {
        if ((n++ & 63) == 0 && stale(gen, expect))
            return false;
        const QString filePath = FilterMatch::joinPath(path, _q(file->getName()));
        const QString hash = tth ? _q(file->getTTH().toBase32()) : QString();
        if (filePasses(filePath, file->getSize(), hash))
            return true;
    }
    for (const auto &sub : dir->directories) {
        if (subtreeHasMatch(sub, FilterMatch::joinPath(path, _q(sub->getName())), gen, expect))
            return true;
    }
    return false;
}

bool ListingMatch::subtreeHasVisibleDir(DirectoryListing::Directory *dir, const QString &path,
                                        const std::atomic<int> *gen, int expect) const
{
    if (!dir || stale(gen, expect))
        return false;
    // Avoid getTotalSize tree walk unless the size filter needs it.
    qulonglong sz = 0;
    if (filter.sizeLimit && filter.sizeMode != SearchManager::SIZE_DONTCARE) {
        sz = static_cast<qulonglong>(dir->getTotalSize(true));
        if (stale(gen, expect))
            return false;
    }
    if (filter.dirPasses(path, sz))
        return true;
    for (const auto &sub : dir->directories) {
        if (subtreeHasVisibleDir(sub, FilterMatch::joinPath(path, _q(sub->getName())), gen, expect))
            return true;
    }
    return false;
}

bool ListingMatch::acceptItem(FileBrowserItem *item, const QString &pathPrefix,
                              const std::atomic<int> *gen, int expect) const
{
    if (!item || stale(gen, expect))
        return false;

    const QString name = item->data(COLUMN_FILEBROWSER_NAME).toString();
    const bool flatRow = item->columnCount() > COLUMN_FILEBROWSER_PATH;
    QString path = flatRow ? item->data(COLUMN_FILEBROWSER_PATH).toString() : pathPrefix;
    if (path.endsWith(QLatin1Char('\\')) || path.endsWith(QLatin1Char('/')))
        path.chop(1);
    const qulonglong size = item->data(COLUMN_FILEBROWSER_ESIZE).toULongLong();

    if (item->dir) {
        if (filter.filesOnly)
            return false;
        const QString full = FilterMatch::joinPath(path, name);
        if (filter.dirsOnly) {
            if (filter.dirPasses(full, size))
                return true;
            // Children only — this dir already tested with the row's ESIZE.
            for (const auto &sub : item->dir->directories) {
                if (subtreeHasVisibleDir(sub, FilterMatch::joinPath(full, _q(sub->getName())),
                                         gen, expect))
                    return true;
            }
            return false;
        }
        return subtreeHasMatch(item->dir, full, gen, expect);
    }

    const QString tth = filter.needTth() ? item->data(COLUMN_FILEBROWSER_TTH).toString() : QString();
    return filter.acceptFile(name, path, tth, size);
}
