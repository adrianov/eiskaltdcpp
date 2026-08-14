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

struct ColSpan {
    int col = 0;
    int soft = 0;
    int full = 0;
};

struct Measure {
    QVector<ColSpan> autos;
    int sumFull = 0;
};

Measure measure(QAbstractItemView *view, QHeaderView *header, const QSet<int> &manual,
                const QHash<int, ColumnWidths> &peaks)
{
    Measure m;
    ColumnContentSpan span(view);
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        if (manual.contains(col)) {
            m.sumFull += header->sectionSize(col);
            continue;
        }
        const ColumnWidths remembered = peaks.value(col);
        ColumnWidths w;
        if (remembered.full > 0) {
            const int t = span.title(col);
            w = ColumnWidths{qMax(remembered.soft, t), qMax(remembered.full, t)};
        } else {
            w = span.widths(col);
        }
        m.autos.append(ColSpan{col, w.soft, w.full});
        m.sumFull += w.full;
    }
    return m;
}

void setContent(QHeaderView *header, const Measure &m, bool useFull)
{
    for (const ColSpan &c : m.autos)
        header->resizeSection(c.col, useFull ? c.full : c.soft);
}

void enlargeProportional(QHeaderView *header, const QSet<int> &manual, int viewW)
{
    QVector<int> cols;
    QVector<int> sizes;
    int total = 0;
    int flexSum = 0;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        const int sz = header->sectionSize(col);
        total += sz;
        if (manual.contains(col))
            continue;
        cols.append(col);
        sizes.append(sz);
        flexSum += sz;
    }
    if (cols.isEmpty() || flexSum < 1 || total >= viewW)
        return;

    const int budget = flexSum + (viewW - total);
    int allocated = 0;
    for (int i = 0; i < cols.size(); ++i) {
        int neu = (i + 1 == cols.size())
                ? budget - allocated
                : int((qint64(sizes.at(i)) * budget) / flexSum);
        neu = qMax(neu, header->minimumSectionSize());
        header->resizeSection(cols.at(i), neu);
        allocated += neu;
    }
}

QHash<int, ColumnWidths> peaksOf(const Measure &m)
{
    QHash<int, ColumnWidths> out;
    out.reserve(m.autos.size());
    for (const ColSpan &c : m.autos)
        out.insert(c.col, ColumnWidths{c.soft, c.full});
    return out;
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

QHash<int, ColumnWidths> HeaderContentFit::apply(const QHash<int, ColumnWidths> &peaks)
{
    QHeaderView *header = headerOf(view_);
    if (!header || !view_->viewport() || header->count() < 1)
        return {};
    header->setStretchLastSection(false);

    const Measure m = measure(view_, header, manual_, peaks);
    if (m.autos.isEmpty())
        return {};

    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (!header->isSectionHidden(col))
            header->setSectionResizeMode(col, QHeaderView::Interactive);
    }

    const int viewW = view_->viewport()->width();
    setContent(header, m, m.sumFull <= viewW);
    enlargeProportional(header, manual_, viewW);
    return peaksOf(m);
}
