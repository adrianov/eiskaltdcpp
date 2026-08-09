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

#include "treeheader/HeaderContentFit.h"
#include "treeheader/ColumnContentSpan.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QVector>

namespace {

constexpr int kMaxOverflowPct = 30;

struct ColNeed {
    int col = 0;
    int need = 0;
};

} // namespace

HeaderContentFit::HeaderContentFit(QAbstractItemView *view, const QSet<int> &manual)
    : view_(view)
    , manual_(manual)
{
}

QHeaderView *HeaderContentFit::headerOf(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *tw = qobject_cast<QTreeWidget*>(view))
        return tw->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
}

bool HeaderContentFit::ready() const
{
    return view_ && view_->isVisible() && view_->viewport()
        && view_->viewport()->width() >= 40 && headerOf(view_);
}

void HeaderContentFit::scaleToViewport(QHeaderView *header, int viewW) const
{
    QVector<int> cols;
    QVector<int> sizes;
    int total = 0;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        cols.append(col);
        const int sz = header->sectionSize(col);
        sizes.append(sz);
        total += sz;
    }
    if (cols.isEmpty() || total <= viewW)
        return;
    // Only when overflow is modest: total <= viewport + 30%.
    if (total * 100LL > viewW * (100LL + kMaxOverflowPct))
        return;

    const int minSec = header->minimumSectionSize();
    int allocated = 0;
    for (int i = 0; i < cols.size(); ++i) {
        int neu = (i + 1 == cols.size())
                ? viewW - allocated
                : int((qint64(sizes.at(i)) * viewW) / total);
        neu = qMax(neu, minSec);
        header->resizeSection(cols.at(i), neu);
        allocated += neu;
    }
}

void HeaderContentFit::apply()
{
    QHeaderView *header = headerOf(view_);
    if (!header || !view_->viewport() || header->count() < 1)
        return;
    header->setStretchLastSection(false);

    ColumnContentSpan span(view_);
    QVector<ColNeed> slackCols;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col) || manual_.contains(col))
            continue;
        const int need = span.cells(col);
        int have = header->sectionSize(col);
        if (have < need) {
            header->resizeSection(col, need);
            have = need;
        }
        if (have > need)
            slackCols.append(ColNeed{col, need});
    }

    const int viewW = view_->viewport()->width();
    if (!slackCols.isEmpty() && header->length() > viewW) {
        for (const ColNeed &item : slackCols)
            header->resizeSection(item.col, item.need);
    }
    scaleToViewport(header, viewW);
}
