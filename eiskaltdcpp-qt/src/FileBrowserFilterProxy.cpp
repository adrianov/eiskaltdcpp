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

#include "dcpp/SearchManager.h"
#include "dcpp/Util.h"

#include <QFileInfo>

using namespace dcpp;

FileBrowserFilterProxy::FileBrowserFilterProxy(bool treeMode, QObject *parent):
    QSortFilterProxyModel(parent),
    treeMode_(treeMode)
{
    setDynamicSortFilter(false);
}

void FileBrowserFilterProxy::sort(int column, Qt::SortOrder order) {
    if (sourceModel())
        sourceModel()->sort(column, order);
}

void FileBrowserFilterProxy::applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                                          bool dirsOnly, bool filesOnly, const QStringList &exts,
                                          const QString &pathPrefix) {
    if (textTermsRaw_ == terms && sizeLimit_ == size && sizeMode_ == sizeMode
            && dirsOnly_ == dirsOnly && filesOnly_ == filesOnly && extFilter_ == exts
            && pathPrefix_ == pathPrefix)
        return;

    if (textTermsRaw_ != terms) {
        textTermsRaw_ = terms;
        textTerms_.clear();
        textTerms_.reserve(static_cast<size_t>(terms.size()));
        for (const QString &term : terms) {
            const string t = _tq(term);
            if (t.empty())
                continue;
            if (t[0] == '-') {
                if (t.size() == 1)
                    continue;
                textTerms_.push_back({t.substr(1), true});
            } else {
                textTerms_.push_back({t, false});
            }
        }
    }

    sizeLimit_ = size;
    sizeMode_ = sizeMode;
    dirsOnly_ = dirsOnly;
    filesOnly_ = filesOnly;
    extFilter_ = exts;
    pathPrefix_ = pathPrefix;
    invalidateFilter();
}

bool FileBrowserFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    if (!filtersActive())
        return true;

    const QModelIndex nameIndex = model->index(sourceRow, COLUMN_FILEBROWSER_NAME, sourceParent);
    const FileBrowserItem *item = static_cast<FileBrowserItem*>(nameIndex.internalPointer());
    if (!item)
        return false;

    if (treeMode_) {
        if (!item->dir)
            return true;

        const FileBrowserModel *fbModel = qobject_cast<const FileBrowserModel*>(model);
        const QString path = fbModel ? fbModel->createRemotePath(const_cast<FileBrowserItem*>(item)) : QString();
        return dirsOnly_ ? subtreeHasVisibleDir(item->dir, path) : subtreeHasMatch(item->dir, path);
    }

    const bool isDir = (item->dir != nullptr);

    if (isDir) {
        const QString dirPath = pathPrefix_.isEmpty()
                ? model->data(nameIndex).toString()
                : pathPrefix_ + QLatin1Char('\\') + model->data(nameIndex).toString();
        const qulonglong rowSize = model->data(
                model->index(sourceRow, COLUMN_FILEBROWSER_ESIZE, sourceParent)).toULongLong();
        if (dirsOnly_)
            return dirPasses(dirPath, rowSize) || subtreeHasVisibleDir(item->dir, dirPath);
        if (filesOnly_)
            return false;
        return subtreeHasMatch(item->dir, dirPath);
    }

    if (dirsOnly_)
        return false;

    if (!textTerms_.empty()) {
        QString hay = model->data(nameIndex).toString();
        if (!pathPrefix_.isEmpty())
            hay = pathPrefix_ + QLatin1Char('\\') + hay;
        hay += QLatin1Char(' ') + model->data(model->index(sourceRow, COLUMN_FILEBROWSER_TTH, sourceParent)).toString();

        if (!matchesText(_tq(hay)))
            return false;
    }

    if (!extFilter_.isEmpty()) {
        const QString ext = QFileInfo(model->data(nameIndex).toString()).suffix().toUpper();
        if (ext.isEmpty() || !extFilter_.contains(ext, Qt::CaseInsensitive))
            return false;
    }

    if (sizeLimit_ && sizeMode_ != SearchManager::SIZE_DONTCARE) {
        const qulonglong rowSize = model->data(model->index(sourceRow, COLUMN_FILEBROWSER_ESIZE, sourceParent)).toULongLong();
        if (sizeMode_ == SearchManager::SIZE_ATLEAST && rowSize < sizeLimit_)
            return false;
        if (sizeMode_ == SearchManager::SIZE_ATMOST && rowSize > sizeLimit_)
            return false;
    }

    return true;
}
