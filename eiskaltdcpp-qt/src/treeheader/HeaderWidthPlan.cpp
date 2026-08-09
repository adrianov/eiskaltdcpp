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

#include "treeheader/HeaderWidthPlan.h"
#include "treeheader/ColumnContentSpan.h"

#include <QAbstractItemView>
#include <QHeaderView>

namespace {
constexpr int kMaxOverflowPct = 30;
}

HeaderWidthPlan::HeaderWidthPlan(QAbstractItemView *view, const QSet<int> &manual)
    : view_(view)
    , manual_(manual)
{
}

bool HeaderWidthPlan::measure(QHeaderView *header)
{
    autos_.clear();
    sumSoft_ = sumFull_ = 0;
    if (!header || !view_)
        return false;

    ColumnContentSpan span(view_);
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        if (manual_.contains(col)) {
            const int sz = header->sectionSize(col);
            sumSoft_ += sz;
            sumFull_ += sz;
            continue;
        }
        const ColumnWidths w = span.widths(col);
        autos_.append(Col{col, w.soft, w.full, span.title(col)});
        sumSoft_ += w.soft;
        sumFull_ += w.full;
    }
    return !autos_.isEmpty();
}

int HeaderWidthPlan::widest() const
{
    int best = -1;
    int bestFull = -1;
    for (const Col &c : autos_) {
        if (c.full > bestFull) {
            bestFull = c.full;
            best = c.col;
        }
    }
    return best;
}

void HeaderWidthPlan::setSoft(QHeaderView *header) const
{
    for (const Col &c : autos_)
        header->resizeSection(c.col, c.soft);
}

void HeaderWidthPlan::setFull(QHeaderView *header) const
{
    for (const Col &c : autos_)
        header->resizeSection(c.col, c.full);
}

void HeaderWidthPlan::scale(QHeaderView *header, const QVector<int> &cols,
                            const QVector<int> &sizes, int total, int budget) const
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

void HeaderWidthPlan::fillSpare(QHeaderView *header, int room) const
{
    if (room < 1)
        return;

    // Side columns reach p100 first; the widest column (Name in trees) takes the rest.
    const int stretch = widest();
    for (const Col &c : autos_) {
        if (c.col == stretch || room < 1)
            continue;
        const int gap = c.full - header->sectionSize(c.col);
        if (gap < 1)
            continue;
        const int add = qMin(gap, room);
        header->resizeSection(c.col, header->sectionSize(c.col) + add);
        room -= add;
    }
    if (stretch >= 0 && room > 0)
        header->resizeSection(stretch, header->sectionSize(stretch) + room);
}

void HeaderWidthPlan::trimWidest(QHeaderView *header, int over) const
{
    if (over < 1)
        return;
    const int stretch = widest();
    for (const Col &c : autos_) {
        if (c.col != stretch)
            continue;
        const int cut = qMin(over, qMax(0, header->sectionSize(c.col) - c.floor));
        if (cut > 0)
            header->resizeSection(c.col, header->sectionSize(c.col) - cut);
        return;
    }
}

void HeaderWidthPlan::shrinkOverflow(QHeaderView *header, int viewW) const
{
    QVector<int> cols;
    QVector<int> sizes;
    int total = 0;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        cols.append(col);
        sizes.append(header->sectionSize(col));
        total += sizes.last();
    }
    if (cols.isEmpty() || total <= viewW)
        return;
    if (total * 100LL > viewW * (100LL + kMaxOverflowPct))
        return;
    scale(header, cols, sizes, total, viewW);
}
