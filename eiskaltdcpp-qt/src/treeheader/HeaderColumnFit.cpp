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

#include "treeheader/HeaderColumnFit.h"
#include "treeheader/ColumnContentSpan.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>

HeaderColumnFit::HeaderColumnFit(QAbstractItemView *view, int stretchColumn,
                                 const QSet<int> &manual)
    : view_(view)
    , stretchColumn_(stretchColumn)
    , manual_(manual)
{
}

QHeaderView *HeaderColumnFit::headerOf(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *tw = qobject_cast<QTreeWidget*>(view))
        return tw->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
}

bool HeaderColumnFit::ready() const
{
    return view_ && view_->isVisible() && view_->viewport()
        && view_->viewport()->width() >= 40 && headerOf(view_);
}

QList<int> HeaderColumnFit::visibleColumns() const
{
    QList<int> visible;
    QHeaderView *header = headerOf(view_);
    if (!header)
        return visible;
    for (int v = 0; v < header->count(); ++v) {
        const int i = header->logicalIndex(v);
        if (!header->isSectionHidden(i))
            visible.append(i);
    }
    return visible;
}

int HeaderColumnFit::flexIndex(const QList<int> &visible) const
{
    const int preferred = visible.indexOf(stretchColumn_);
    if (preferred >= 0 && !manual_.contains(stretchColumn_))
        return preferred;
    for (int i = 0; i < visible.size(); ++i) {
        if (!manual_.contains(visible.at(i)))
            return i;
    }
    return -1;
}

bool HeaderColumnFit::fillsView() const
{
    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return true;
    if (!ready())
        return false;

    QHeaderView *header = headerOf(view_);
    ColumnContentSpan span(view_);
    int total = 0;
    for (int col : visible) {
        if (!manual_.contains(col) && header->sectionSize(col) < span.title(col))
            return false;
        total += header->sectionSize(col);
    }
    // Restored or dragged widths that already fill the viewport need no refit.
    return total >= view_->viewport()->width() - 32;
}

QVector<int> HeaderColumnFit::baseWidths(const QList<int> &visible,
                                         QHeaderView *header) const
{
    ColumnContentSpan span(view_);
    const bool hasRows = view_->model() && view_->model()->rowCount() > 0;
    QVector<int> widths;
    widths.reserve(visible.size());
    for (int col : visible) {
        const int prev = header->sectionSize(col);
        if (manual_.contains(col))
            widths.append(prev);
        else
            widths.append(qMax(hasRows ? span.cells(col) : span.title(col), prev));
    }
    return widths;
}

void HeaderColumnFit::balance(QVector<int> &widths, const QList<int> &visible,
                              int viewWidth) const
{
    int total = 0;
    for (int w : widths)
        total += w;

    const int flex = flexIndex(visible);
    if (flex >= 0 && total < viewWidth - 8) {
        widths[flex] += viewWidth - total;
        return;
    }
    if (total <= viewWidth || stretchColumn_ < 0 || manual_.contains(stretchColumn_))
        return;
    const int at = visible.indexOf(stretchColumn_);
    if (at < 0)
        return;
    ColumnContentSpan span(view_);
    const int cut = qMin(total - viewWidth,
                         qMax(0, widths[at] - span.title(stretchColumn_)));
    if (cut > 0)
        widths[at] -= cut;
}

void HeaderColumnFit::apply()
{
    QHeaderView *header = headerOf(view_);
    if (!ready() || header->count() < 1)
        return;
    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return;

    header->setStretchLastSection(false);
    // Snapshot sizes inside baseWidths before cells() probes with resizeColumnToContents.
    QVector<int> widths = baseWidths(visible, header);
    balance(widths, visible, view_->viewport()->width());
    for (int i = 0; i < visible.size(); ++i)
        header->resizeSection(visible.at(i), widths.at(i));
}
