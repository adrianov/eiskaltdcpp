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

#include "filebrowser/FilterMatch.h"
#include "FileBrowserModel.h"
#include "WulforUtil.h"

#include "dcpp/SearchManager.h"

using namespace dcpp;

namespace {

QString joinPath(const QString &path, const QString &name)
{
    if (path.isEmpty())
        return name;
    if (path.endsWith(QLatin1Char('\\')))
        return path + name;
    return path + QLatin1Char('\\') + name;
}

bool stale(const std::atomic<int> *gen, int expect)
{
    return gen && gen->load() != expect;
}

} // namespace

bool FilterMatch::needTth() const
{
    for (const Term &term : terms) {
        if (!term.exclude && term.value.size() >= 39)
            return true;
    }
    return false;
}

bool FilterMatch::matchesText(const QString &haystack) const
{
    for (const Term &term : terms) {
        const bool found = haystack.contains(term.value, Qt::CaseInsensitive);
        if (term.exclude ? found : !found)
            return false;
    }
    return true;
}

bool FilterMatch::isActive() const
{
    const bool noText = terms.empty();
    const bool noType = !dirsOnly && !filesOnly && extFilter.isEmpty();
    const bool noSize = !sizeLimit || sizeMode == SearchManager::SIZE_DONTCARE;
    return !(noText && noType && noSize);
}

void FilterMatch::setTerms(const QStringList &raw)
{
    terms.clear();
    terms.reserve(static_cast<size_t>(raw.size()));
    for (const QString &term : raw) {
        if (term.isEmpty())
            continue;
        if (term.at(0) == QLatin1Char('-')) {
            if (term.size() == 1)
                continue;
            terms.push_back({term.mid(1), true});
        } else {
            terms.push_back({term, false});
        }
    }
}

bool FilterMatch::dirPasses(const QString &path, qulonglong size) const
{
    if (!dirsOnly)
        return false;
    if (sizeLimit && sizeMode != SearchManager::SIZE_DONTCARE) {
        if (sizeMode == SearchManager::SIZE_ATLEAST && size < sizeLimit)
            return false;
        if (sizeMode == SearchManager::SIZE_ATMOST && size > sizeLimit)
            return false;
    }
    return terms.empty() || matchesText(path);
}

bool FilterMatch::acceptFile(const QString &name, const QString &path, const QString &tth,
                             qulonglong size) const
{
    if (dirsOnly)
        return false;

    if (!extFilter.isEmpty()) {
        const int dot = name.lastIndexOf(QLatin1Char('.'));
        if (dot < 0 || !extFilter.contains(name.mid(dot + 1).toUpper()))
            return false;
    }

    if (sizeLimit && sizeMode != SearchManager::SIZE_DONTCARE) {
        if (sizeMode == SearchManager::SIZE_ATLEAST && size < sizeLimit)
            return false;
        if (sizeMode == SearchManager::SIZE_ATMOST && size > sizeLimit)
            return false;
    }

    if (terms.empty())
        return true;

    QString hay = joinPath(path, name);
    if (!tth.isEmpty()) {
        hay += QLatin1Char(' ');
        hay += tth;
    }
    return matchesText(hay);
}

bool FilterMatch::filePasses(const QString &filePath, qulonglong size, const QString &tth) const
{
    const int slash = filePath.lastIndexOf(QLatin1Char('\\'));
    const QString name = slash >= 0 ? filePath.mid(slash + 1) : filePath;
    const QString path = slash >= 0 ? filePath.left(slash) : QString();
    return acceptFile(name, path, tth, size);
}

bool FilterMatch::subtreeHasMatch(DirectoryListing::Directory *dir, const QString &path,
                                  const std::atomic<int> *gen, int expect) const
{
    if (!dir || stale(gen, expect))
        return false;
    // Dir path can match on its own (empty dirs / tree parents); skip when ext-only.
    if (!terms.empty() && extFilter.isEmpty() && matchesText(path))
        return true;
    int n = 0;
    for (const auto &file : dir->files) {
        if ((n++ & 63) == 0 && stale(gen, expect))
            return false;
        const QString filePath = joinPath(path, _q(file->getName()));
        if (filePasses(filePath, file->getSize(), _q(file->getTTH().toBase32())))
            return true;
    }
    for (const auto &sub : dir->directories) {
        if (subtreeHasMatch(sub, joinPath(path, _q(sub->getName())), gen, expect))
            return true;
    }
    return false;
}

bool FilterMatch::subtreeHasVisibleDir(DirectoryListing::Directory *dir, const QString &path,
                                       const std::atomic<int> *gen, int expect) const
{
    if (!dir || stale(gen, expect))
        return false;
    if (dirPasses(path, dir->getTotalSize(true)))
        return true;
    for (const auto &sub : dir->directories) {
        if (subtreeHasVisibleDir(sub, joinPath(path, _q(sub->getName())), gen, expect))
            return true;
    }
    return false;
}

bool FilterMatch::acceptItem(FileBrowserItem *item, const QString &pathPrefix,
                             const std::atomic<int> *gen, int expect) const
{
    if (!item || stale(gen, expect))
        return false;

    const QString name = item->data(COLUMN_FILEBROWSER_NAME).toString();
    const bool flatRow = item->columnCount() > COLUMN_FILEBROWSER_PATH;
    QString path = flatRow ? item->data(COLUMN_FILEBROWSER_PATH).toString() : pathPrefix;
    if (path.endsWith(QLatin1Char('\\')))
        path.chop(1);
    const qulonglong size = item->data(COLUMN_FILEBROWSER_ESIZE).toULongLong();

    if (item->dir) {
        if (filesOnly)
            return false;
        const QString full = joinPath(path, name);
        if (dirsOnly)
            return dirPasses(full, size) || subtreeHasVisibleDir(item->dir, full, gen, expect);
        return subtreeHasMatch(item->dir, full, gen, expect);
    }

    const QString tth = needTth() ? item->data(COLUMN_FILEBROWSER_TTH).toString() : QString();
    return acceptFile(name, path, tth, size);
}
