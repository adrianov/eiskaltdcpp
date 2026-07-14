/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserFilterProxy.h"
#include "WulforUtil.h"

#include "dcpp/DirectoryListing.h"
#include "dcpp/SearchManager.h"
#include "dcpp/Util.h"

#include <QFileInfo>

using namespace dcpp;

bool FileBrowserFilterProxy::filtersActive() const {
    const bool noText = textTerms_.empty();
    const bool noType = !dirsOnly_ && !filesOnly_ && extFilter_.isEmpty();
    const bool noSize = !sizeLimit_ || sizeMode_ == SearchManager::SIZE_DONTCARE;
    return !(noText && noType && noSize);
}

bool FileBrowserFilterProxy::matchesText(const std::string &haystack) const {
    for (const Term &term : textTerms_) {
        const bool found = Util::findSubString(haystack, term.value) != string::npos;
        if (term.exclude ? found : !found)
            return false;
    }
    return true;
}

bool FileBrowserFilterProxy::filePasses(const QString &filePath, qulonglong size, const QString &tth) const {
    if (dirsOnly_)
        return false;

    if (filesOnly_ || !extFilter_.isEmpty()) {
        const QString ext = QFileInfo(filePath.section(QLatin1Char('\\'), -1)).suffix().toUpper();
        if (ext.isEmpty() || !extFilter_.contains(ext, Qt::CaseInsensitive))
            return false;
    }

    if (sizeLimit_ && sizeMode_ != SearchManager::SIZE_DONTCARE) {
        if (sizeMode_ == SearchManager::SIZE_ATLEAST && size < sizeLimit_)
            return false;
        if (sizeMode_ == SearchManager::SIZE_ATMOST && size > sizeLimit_)
            return false;
    }

    if (!textTerms_.empty()) {
        QString hay = filePath;
        if (!tth.isEmpty())
            hay += QLatin1Char(' ') + tth;
        if (!matchesText(_tq(hay)))
            return false;
    }

    return true;
}

bool FileBrowserFilterProxy::dirPasses(const QString &path, qulonglong size) const {
    if (!dirsOnly_)
        return false;

    if (sizeLimit_ && sizeMode_ != SearchManager::SIZE_DONTCARE) {
        if (sizeMode_ == SearchManager::SIZE_ATLEAST && size < sizeLimit_)
            return false;
        if (sizeMode_ == SearchManager::SIZE_ATMOST && size > sizeLimit_)
            return false;
    }

    if (!textTerms_.empty() && !matchesText(_tq(path)))
        return false;

    return true;
}

bool FileBrowserFilterProxy::subtreeHasMatch(DirectoryListing::Directory *dir,
                                             const QString &path) const {
    if (!dir)
        return false;

    for (const auto &file : dir->files) {
        const QString filePath = path.isEmpty()
                ? _q(file->getName())
                : path + QLatin1Char('\\') + _q(file->getName());
        if (filePasses(filePath, file->getSize(), _q(file->getTTH().toBase32())))
            return true;
    }

    for (const auto &sub : dir->directories) {
        const QString subPath = path.isEmpty()
                ? _q(sub->getName())
                : path + QLatin1Char('\\') + _q(sub->getName());
        if (subtreeHasMatch(sub, subPath))
            return true;
    }

    return false;
}

bool FileBrowserFilterProxy::subtreeHasVisibleDir(DirectoryListing::Directory *dir,
                                                  const QString &path) const {
    if (!dir)
        return false;

    if (dirPasses(path, dir->getTotalSize(true)))
        return true;

    for (const auto &sub : dir->directories) {
        const QString subPath = path.isEmpty()
                ? _q(sub->getName())
                : path + QLatin1Char('\\') + _q(sub->getName());
        if (subtreeHasVisibleDir(sub, subPath))
            return true;
    }

    return false;
}
