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

#include "treeheader/ColumnContentSpan.h"
#include "treeheader/HeaderColumnFit.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QTableView>
#include <QTreeView>

namespace {

int iconPad(const QVariant &deco)
{
    if (deco.canConvert<QPixmap>())
        return deco.value<QPixmap>().width() + 4;
    if (!deco.canConvert<QIcon>())
        return 0;
    const QIcon icon = deco.value<QIcon>();
    return icon.isNull() ? 0 : icon.actualSize(QSize(16, 16)).width() + 4;
}

void walkCells(QAbstractItemModel *model, const QModelIndex &parent,
               int column, const QFontMetrics &fm, int depth, int &w)
{
    if (!model || depth > 24)
        return;
    const int rows = qMin(model->rowCount(parent), 300);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, column, parent);
        if (!idx.isValid())
            continue;
        w = qMax(w, fm.horizontalAdvance(idx.data(Qt::DisplayRole).toString())
                 + 20 + iconPad(idx.data(Qt::DecorationRole)));
        if (model->hasChildren(idx))
            walkCells(model, idx, column, fm, depth + 1, w);
    }
}

} // namespace

ColumnContentSpan::ColumnContentSpan(QAbstractItemView *view)
    : view_(view)
{
}

int ColumnContentSpan::title(int column) const
{
    QAbstractItemModel *model = view_ ? view_->model() : nullptr;
    if (!model)
        return 48;
    return qMax(48, QFontMetrics(view_->font()).horizontalAdvance(
                   model->headerData(column, Qt::Horizontal).toString()) + 24);
}

int ColumnContentSpan::cells(int column) const
{
    QHeaderView *header = HeaderColumnFit::headerOf(view_);
    int w = title(column);
    if (QTreeView *tree = qobject_cast<QTreeView*>(view_))
        tree->resizeColumnToContents(column);
    else if (QTableView *table = qobject_cast<QTableView*>(view_))
        table->resizeColumnToContents(column);
    if (header) {
        w = qMax(w, header->sectionSize(column));
        w = qMax(w, header->sectionSizeHint(column) + 16);
    }
    if (QAbstractItemModel *model = view_->model())
        walkCells(model, QModelIndex(), column, QFontMetrics(view_->font()), 0, w);
    return w;
}
