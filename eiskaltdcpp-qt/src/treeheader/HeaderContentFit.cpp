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

struct ColSpan {
    int col = 0;
    int soft = 0;
    int full = 0;
};

void scaleColumns(QHeaderView *header, const QVector<int> &cols,
                  const QVector<int> &sizes, int total, int budget)
{
    if (cols.isEmpty() || total < 1 || budget < 1 || total == budget)
        return;
    const int minSec = header->minimumSectionSize();
    int allocated = 0;
    for (int i = 0; i < cols.size(); ++i) {
        int neu = (i + 1 == cols.size())
                ? budget - allocated
                : int((qint64(sizes.at(i)) * budget) / total);
        neu = qMax(neu, minSec);
        header->resizeSection(cols.at(i), neu);
        allocated += neu;
    }
}

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
    QVector<int> allCols;
    QVector<int> allSizes;
    QVector<int> flexCols;
    QVector<int> flexSizes;
    int total = 0;
    int flexSum = 0;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        const int sz = header->sectionSize(col);
        allCols.append(col);
        allSizes.append(sz);
        total += sz;
        if (!manual_.contains(col)) {
            flexCols.append(col);
            flexSizes.append(sz);
            flexSum += sz;
        }
    }
    if (allCols.isEmpty() || total < 1 || total == viewW)
        return;

    if (total > viewW) {
        // Modest overflow: shrink every column (including manual).
        if (total * 100LL > viewW * (100LL + kMaxOverflowPct))
            return;
        scaleColumns(header, allCols, allSizes, total, viewW);
        return;
    }

    // Free space: enlarge non-manual columns only so user drags stay put.
    if (flexSum < 1)
        return;
    scaleColumns(header, flexCols, flexSizes, flexSum, flexSum + (viewW - total));
}

void HeaderContentFit::apply()
{
    QHeaderView *header = headerOf(view_);
    if (!header || !view_->viewport() || header->count() < 1)
        return;
    header->setStretchLastSection(false);

    const int viewW = view_->viewport()->width();
    ColumnContentSpan measure(view_);
    QVector<ColSpan> autos;
    int sumFull = 0;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        if (manual_.contains(col)) {
            sumFull += header->sectionSize(col);
            continue;
        }
        const ColumnWidths w = measure.widths(col);
        autos.append(ColSpan{col, w.soft, w.full});
        sumFull += w.full;
    }

    const bool useFull = sumFull <= viewW;
    for (const ColSpan &item : autos)
        header->resizeSection(item.col, useFull ? item.full : item.soft);
    scaleToViewport(header, viewW);
}
