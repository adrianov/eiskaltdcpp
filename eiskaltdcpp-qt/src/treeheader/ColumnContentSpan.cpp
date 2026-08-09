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
/** Prefer p100 when it is within this fraction of p80 (no tall outliers). */
constexpr int kNearMaxPct = 20;

int iconPad(const QVariant &deco)
{
    if (deco.canConvert<QPixmap>())
        return deco.value<QPixmap>().width() + 4;
    if (!deco.canConvert<QIcon>())
        return 0;
    const QIcon icon = deco.value<QIcon>();
    return icon.isNull() ? 0 : icon.actualSize(QSize(16, 16)).width() + 4;
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
        if (!idx.isValid())
            continue;
        const QString text = idx.data(Qt::DisplayRole).toString();
        // Empty cells are not samples — they must not pull p80/p100 down.
        if (!text.isEmpty()) {
            out.append(fm.horizontalAdvance(text)
                       + 20 + iconPad(idx.data(Qt::DecorationRole))
                       + (column == 0 ? depth * indent : 0));
        }
        if (model->hasChildren(idx))
            collectWidths(model, idx, column, fm, depth + 1, indent, out);
    }
}

int contentWidth(QVector<int> &widths)
{
    if (widths.isEmpty())
        return 0;
    std::sort(widths.begin(), widths.end());
    const int p80 = widths.at(qBound(0, (widths.size() * kPercentile + 99) / 100 - 1,
                                     widths.size() - 1));
    const int p100 = widths.last();
    // p100 if at most 20% wider than p80 (p100 / p80 <= 1.2).
    if (p100 * 100LL <= p80 * (100LL + kNearMaxPct))
        return p100;
    return p80;
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
    const int floor = title(column);
    QAbstractItemModel *model = view_ ? view_->model() : nullptr;
    if (!model || model->rowCount() < 1)
        return floor;

    int indent = 0;
    if (QTreeView *tree = qobject_cast<QTreeView*>(view_))
        indent = tree->indentation();

    QVector<int> widths;
    widths.reserve(qMin(model->rowCount(), kSampleRows));
    collectWidths(model, QModelIndex(), column, QFontMetrics(view_->font()),
                  0, indent, widths);
    return qMax(floor, contentWidth(widths));
}
