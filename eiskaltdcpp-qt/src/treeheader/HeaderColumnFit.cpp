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

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QVector>

namespace {

void measureRows(QAbstractItemModel *model, const QModelIndex &parent,
                 int column, const QFontMetrics &fm, int depth, int &w)
{
    if (!model || depth > 24)
        return;
    const int rows = qMin(model->rowCount(parent), 300);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, column, parent);
        if (!idx.isValid())
            continue;
        int rowW = fm.horizontalAdvance(idx.data(Qt::DisplayRole).toString()) + 20;
        const QVariant deco = idx.data(Qt::DecorationRole);
        if (deco.canConvert<QPixmap>())
            rowW += deco.value<QPixmap>().width() + 4;
        else if (deco.canConvert<QIcon>()) {
            const QIcon icon = deco.value<QIcon>();
            if (!icon.isNull())
                rowW += icon.actualSize(QSize(16, 16)).width() + 4;
        }
        w = qMax(w, rowW);
        if (model->hasChildren(idx))
            measureRows(model, idx, column, fm, depth + 1, w);
    }
}

} // namespace

HeaderColumnFit::HeaderColumnFit(QAbstractItemView *view, int stretchColumn)
    : view_(view)
    , stretchColumn_(stretchColumn)
{
}

QHeaderView *HeaderColumnFit::headerOf(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *treeWidget = qobject_cast<QTreeWidget*>(view))
        return treeWidget->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
}

bool HeaderColumnFit::canApply() const
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

int HeaderColumnFit::labelWidth(int column) const
{
    QAbstractItemModel *model = view_->model();
    if (!model)
        return 48;
    return qMax(48, QFontMetrics(view_->font()).horizontalAdvance(
                   model->headerData(column, Qt::Horizontal).toString()) + 24);
}

int HeaderColumnFit::contentWidth(int column) const
{
    QHeaderView *header = headerOf(view_);
    int w = labelWidth(column);
    if (QTreeView *tree = qobject_cast<QTreeView*>(view_))
        tree->resizeColumnToContents(column);
    else if (QTableView *table = qobject_cast<QTableView*>(view_))
        table->resizeColumnToContents(column);
    w = qMax(w, header->sectionSize(column));
    w = qMax(w, header->sectionSizeHint(column) + 16);
    if (QAbstractItemModel *model = view_->model())
        measureRows(model, QModelIndex(), column, QFontMetrics(view_->font()), 0, w);
    return w;
}

int HeaderColumnFit::stretchIndex(const QList<int> &visible) const
{
    if (visible.isEmpty())
        return 0;
    if (stretchColumn_ >= 0) {
        const int idx = visible.indexOf(stretchColumn_);
        if (idx >= 0)
            return idx;
    }
    return 0;
}

bool HeaderColumnFit::isAdequate() const
{
    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return true;
    if (!canApply())
        return false;

    QHeaderView *header = headerOf(view_);
    const int viewW = view_->viewport()->width();
    int total = 0;
    for (int col : visible) {
        if (header->sectionSize(col) < labelWidth(col))
            return false;
        total += header->sectionSize(col);
    }
    // Saved/user widths that fill the viewport are fine — do not refit to content.
    return total >= viewW - 32;
}

void HeaderColumnFit::apply()
{
    QHeaderView *header = headerOf(view_);
    if (!canApply() || header->count() < 1)
        return;

    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return;

    header->setStretchLastSection(false);
    const int viewWidth = view_->viewport()->width();
    const bool hasRows = view_->model() && view_->model()->rowCount() > 0;

    // Snapshot before contentWidth() mutates sections via resizeColumnToContents.
    QVector<int> previous;
    previous.reserve(visible.size());
    for (int col : visible)
        previous.append(header->sectionSize(col));

    QVector<int> widths;
    widths.reserve(visible.size());
    int total = 0;
    for (int i = 0; i < visible.size(); ++i) {
        const int col = visible.at(i);
        // Keep restored/user widths; with no rows only enforce label minimum.
        const int need = hasRows ? contentWidth(col) : labelWidth(col);
        widths.append(qMax(need, previous.at(i)));
        total += widths.last();
    }

    const int stretch = stretchIndex(visible);
    const int shrinkAt = visible.indexOf(stretchColumn_);
    if (total < viewWidth - 8)
        widths[stretch] += viewWidth - total;
    else if (total > viewWidth && shrinkAt >= 0) {
        const int minW = labelWidth(stretchColumn_);
        const int shrink = qMin(total - viewWidth, qMax(0, widths[shrinkAt] - minW));
        if (shrink > 0)
            widths[shrinkAt] -= shrink;
    }

    for (int i = 0; i < visible.size(); ++i)
        header->resizeSection(visible.at(i), widths.at(i));
}
