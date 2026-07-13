/***************************************************************************
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
    invalidateFilter();
}

bool SearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    const bool noText = textTerms_.empty();
    const bool noType = !dirsOnly_ && !filesOnly_ && extFilter_.isEmpty();
    const bool noSize = !sizeLimit_ || sizeMode_ == SearchManager::SIZE_DONTCARE;
    if (noText && noType && noSize)
        return true;

    const QModelIndex nameIndex = model->index(sourceRow, COLUMN_SF_FILENAME, sourceParent);
    const SearchItem *item = static_cast<SearchItem*>(nameIndex.internalPointer());
    if (!item)
        return true;

    if (!noText) {
        // Same case-insensitive fold as hub intake (Util::findSubString / utf8ToLC).
        const string haystack = _tq(
                model->data(model->index(sourceRow, COLUMN_SF_PATH, sourceParent)).toString() +
                model->data(nameIndex).toString() + QLatin1Char(' ') +
                model->data(model->index(sourceRow, COLUMN_SF_TTH, sourceParent)).toString());
        for (const Term &term : textTerms_) {
            const bool found = Util::findSubString(haystack, term.value) != string::npos;
            if (term.exclude ? found : !found)
                return false;
        }
    }

    if (dirsOnly_) {
        if (!item->isDir)
            return false;
    } else if (filesOnly_ || !extFilter_.isEmpty()) {
        if (item->isDir)
            return false;
        if (!extFilter_.isEmpty()) {
            const QString ext = model->data(model->index(sourceRow, COLUMN_SF_EXTENSION, sourceParent)).toString();
            if (ext.isEmpty() || !extFilter_.contains(ext, Qt::CaseInsensitive))
                return false;
        }
    }

    if (!noSize) {
        const qulonglong size = model->data(model->index(sourceRow, COLUMN_SF_ESIZE, sourceParent)).toULongLong();
        if (sizeMode_ == SearchManager::SIZE_ATLEAST && size < sizeLimit_)
            return false;
        if (sizeMode_ == SearchManager::SIZE_ATMOST && size > sizeLimit_)
            return false;
    }

    return true;
}
