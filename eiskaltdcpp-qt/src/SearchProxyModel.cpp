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

#include "SearchProxyModel.h"
#include "SearchModel.h"
#include "WulforUtil.h"

#include "dcpp/SearchManager.h"
#include "dcpp/Util.h"

using namespace dcpp;

SearchProxyModel::SearchProxyModel(QObject *parent):
    QSortFilterProxyModel(parent)
{
    // false: rowsInserted still runs filterAcceptsRow for new data, but
    // dataChanged (highlights) must not refilter the whole list. Criteria
    // changes call invalidateFilter() via applyFilters().
    setDynamicSortFilter(false);
}

void SearchProxyModel::sort(int column, Qt::SortOrder order){
    if (sourceModel())
        sourceModel()->sort(column, order);
}

void SearchProxyModel::applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                                    bool dirsOnly, bool filesOnly, const QStringList &exts){
    if (textTermsRaw_ == terms && sizeLimit_ == size && sizeMode_ == sizeMode
            && dirsOnly_ == dirsOnly && filesOnly_ == filesOnly && extFilter_ == exts)
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
    WULFOR_INVALIDATE_FILTER();
}

bool SearchProxyModel::acceptText(const QAbstractItemModel *model, int row,
                                 const QModelIndex &parent) const {
    if (textTerms_.empty())
        return true;
    // Same case-insensitive fold as hub intake (Util::findSubString / utf8ToLC).
    const string haystack = _tq(
            model->data(model->index(row, COLUMN_SF_PATH, parent)).toString() +
            model->data(model->index(row, COLUMN_SF_FILENAME, parent)).toString() +
            QLatin1Char(' ') +
            model->data(model->index(row, COLUMN_SF_TTH, parent)).toString());
    for (const Term &term : textTerms_) {
        const bool found = Util::findSubString(haystack, term.value) != string::npos;
        if (term.exclude ? found : !found)
            return false;
    }
    return true;
}

bool SearchProxyModel::acceptType(const SearchItem *item, const QAbstractItemModel *model,
                                  int row, const QModelIndex &parent) const {
    if (dirsOnly_)
        return item->isDir;
    if (!filesOnly_ && extFilter_.isEmpty())
        return true;
    if (item->isDir)
        return false;
    if (extFilter_.isEmpty())
        return true;
    const QString ext = model->data(model->index(row, COLUMN_SF_EXTENSION, parent)).toString();
    return !ext.isEmpty() && extFilter_.contains(ext, Qt::CaseInsensitive);
}

bool SearchProxyModel::acceptSize(const QAbstractItemModel *model, int row,
                                  const QModelIndex &parent) const {
    if (!sizeLimit_ || sizeMode_ == SearchManager::SIZE_DONTCARE)
        return true;
    const qulonglong size = model->data(model->index(row, COLUMN_SF_ESIZE, parent)).toULongLong();
    if (sizeMode_ == SearchManager::SIZE_ATLEAST)
        return size >= sizeLimit_;
    if (sizeMode_ == SearchManager::SIZE_ATMOST)
        return size <= sizeLimit_;
    return true;
}

bool SearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;
    if (textTerms_.empty() && !dirsOnly_ && !filesOnly_ && extFilter_.isEmpty()
            && (!sizeLimit_ || sizeMode_ == SearchManager::SIZE_DONTCARE))
        return true;

    const SearchItem *item = static_cast<SearchItem*>(
            model->index(sourceRow, COLUMN_SF_FILENAME, sourceParent).internalPointer());
    if (!item)
        return false;
    return acceptText(model, sourceRow, sourceParent)
            && acceptType(item, model, sourceRow, sourceParent)
            && acceptSize(model, sourceRow, sourceParent);
}
