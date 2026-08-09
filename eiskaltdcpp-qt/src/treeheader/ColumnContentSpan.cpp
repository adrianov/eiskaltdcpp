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

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QIcon>
#include <QPixmap>
#include <QTreeView>
#include <QVector>

#include <algorithm>

namespace {

constexpr int kSampleRows = 300;
constexpr int kPercentile = 80;
constexpr int kAboveP80Pct = 30;

int iconPad(const QVariant &deco)
{
    if (deco.canConvert<QPixmap>())
        return deco.value<QPixmap>().width() + 4;
    if (!deco.canConvert<QIcon>())
        return 0;
    const QIcon icon = deco.value<QIcon>();
    return icon.isNull() ? 0 : icon.actualSize(QSize(16, 16)).width() + 4;
}

int treeDepth0(QAbstractItemView *view)
{
    QTreeView *tree = qobject_cast<QTreeView*>(view);
    return (tree && tree->rootIsDecorated()) ? 1 : 0;
}

int treeIndent(QAbstractItemView *view)
{
    QTreeView *tree = qobject_cast<QTreeView*>(view);
    return tree ? tree->indentation() : 0;
}

int indexDepth(const QModelIndex &index, int depth0)
{
    int depth = depth0;
    for (QModelIndex p = index.parent(); p.isValid(); p = p.parent())
        ++depth;
    return depth;
}

int cellSpan(const QModelIndex &idx, const QFontMetrics &fm, int depth, int indent)
{
    if (!idx.isValid())
        return 0;
    const QString text = idx.data(Qt::DisplayRole).toString();
    const int icons = iconPad(idx.data(Qt::DecorationRole));
    if (text.trimmed().isEmpty() && icons < 1)
        return 0;
    return fm.horizontalAdvance(text) + 20 + icons
            + (idx.column() == 0 ? depth * indent : 0);
}

void collectWidths(QAbstractItemModel *model, const QModelIndex &parent,
                   int column, const QFontMetrics &fm, int depth, int indent,
                   QVector<int> &out)
{
    if (!model || depth > 24)
        return;
    const int rows = qMin(model->rowCount(parent), kSampleRows);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, column, parent);
        const int w = cellSpan(idx, fm, depth, indent);
        if (w > 0)
            out.append(w);
        if (model->hasChildren(idx))
            collectWidths(model, idx, column, fm, depth + 1, indent, out);
    }
}

ColumnWidths fromSamples(QVector<int> &widths, int floor)
{
    if (widths.isEmpty())
        return ColumnWidths{floor, floor};
    std::sort(widths.begin(), widths.end());
    const int p80 = widths.at(qBound(0, (widths.size() * kPercentile + 99) / 100 - 1,
                                     widths.size() - 1));
    const int p100 = widths.last();
    const int soft = qMin(p80 + (p80 * kAboveP80Pct) / 100, p100);
    return ColumnWidths{qMax(floor, soft), qMax(floor, p100)};
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

int ColumnContentSpan::cellWidth(const QModelIndex &index) const
{
    if (!view_ || !index.isValid())
        return 0;
    return cellSpan(index, QFontMetrics(view_->font()),
                    indexDepth(index, treeDepth0(view_)), treeIndent(view_));
}

ColumnWidths ColumnContentSpan::widths(int column) const
{
    const int floor = title(column);
    QAbstractItemModel *model = view_ ? view_->model() : nullptr;
    if (!model || model->rowCount() < 1)
        return ColumnWidths{floor, floor};

    QVector<int> samples;
    samples.reserve(qMin(model->rowCount(), kSampleRows));
    collectWidths(model, QModelIndex(), column, QFontMetrics(view_->font()),
                  treeDepth0(view_), treeIndent(view_), samples);
    return fromSamples(samples, floor);
}
