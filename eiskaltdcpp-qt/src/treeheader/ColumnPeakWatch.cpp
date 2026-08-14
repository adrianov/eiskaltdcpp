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

#include "treeheader/ColumnPeakWatch.h"
#include "treeheader/HeaderContentFit.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHeaderView>

namespace {
constexpr int kSampleRows = 300; // ColumnContentSpan
constexpr int kShrinkPct = 30;
}

ColumnPeakWatch::ColumnPeakWatch(QAbstractItemView *view)
    : view_(view)
{
}

void ColumnPeakWatch::setManual(const QSet<int> *manual)
{
    manual_ = manual;
}

void ColumnPeakWatch::setNeedFit(NeedFit fn)
{
    needFit_ = std::move(fn);
}

void ColumnPeakWatch::mergePeaks(const QHash<int, ColumnWidths> &measured)
{
    for (auto it = measured.constBegin(); it != measured.constEnd(); ++it) {
        ColumnWidths p = peaks_.value(it.key());
        p.soft = qMax(p.soft, it.value().soft);
        p.full = qMax(p.full, it.value().full);
        peaks_.insert(it.key(), p);
    }
}

void ColumnPeakWatch::clearPeaks()
{
    peaks_.clear();
    pendingRecheck_ = false;
}

void ColumnPeakWatch::recheck()
{
    if (!pendingRecheck_ || !view_)
        return;
    pendingRecheck_ = false;
    QHeaderView *header = HeaderContentFit::headerOf(view_);
    if (!header)
        return;

    ColumnContentSpan span(view_);
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col) || (manual_ && manual_->contains(col)))
            continue;
        const ColumnWidths w = span.widths(col);
        ColumnWidths p = peaks_.value(col);
        if (w.full > p.full || w.soft > p.soft) {
            p.soft = qMax(p.soft, w.soft);
            p.full = qMax(p.full, w.full);
            peaks_.insert(col, p);
        } else if (p.full > 0 && w.full * 100 <= p.full * (100 - kShrinkPct)) {
            peaks_.insert(col, w);
        }
    }
}

void ColumnPeakWatch::fitIfGrew(bool grew)
{
    if (grew && needFit_)
        needFit_();
}

bool ColumnPeakWatch::noteWider(int col, int width)
{
    if (peaks_.isEmpty() || width < 1 || (manual_ && manual_->contains(col)))
        return false;
    ColumnWidths p = peaks_.value(col);
    if (width <= p.full)
        return false;
    p.full = width;
    p.soft = qMax(p.soft, width);
    peaks_.insert(col, p);
    return true;
}

void ColumnPeakWatch::onInserted(const QModelIndex &parent, int first, int last)
{
    if (!view_ || peaks_.isEmpty()) {
        if (needFit_)
            needFit_();
        return;
    }
    QAbstractItemModel *model = view_->model();
    QHeaderView *header = HeaderContentFit::headerOf(view_);
    if (!model || !header)
        return;

    ColumnContentSpan span(view_);
    const int end = qMin(last, first + kSampleRows - 1);
    bool grew = false;
    for (int r = first; r <= end; ++r) {
        for (int v = 0; v < header->count(); ++v) {
            const int col = header->logicalIndex(v);
            if (header->isSectionHidden(col) || (manual_ && manual_->contains(col)))
                continue;
            grew |= noteWider(col, span.cellWidth(model->index(r, col, parent)));
        }
    }
    fitIfGrew(grew);
}

void ColumnPeakWatch::onDataChanged(const QModelIndex &tl, const QModelIndex &br,
                                    const QVector<int> &roles)
{
    if (!roles.isEmpty()
            && !roles.contains(Qt::DisplayRole)
            && !roles.contains(Qt::DecorationRole))
        return;
    if (!view_ || peaks_.isEmpty()) {
        if (needFit_)
            needFit_();
        return;
    }
    ColumnContentSpan span(view_);
    const int end = qMin(br.row(), tl.row() + kSampleRows - 1);
    bool grew = false;
    for (int r = tl.row(); r <= end; ++r) {
        for (int c = tl.column(); c <= br.column(); ++c) {
            const QModelIndex idx = tl.sibling(r, c);
            if (!idx.isValid() || (manual_ && manual_->contains(c)))
                continue;
            grew |= noteWider(c, span.cellWidth(idx));
        }
    }
    fitIfGrew(grew);
}

void ColumnPeakWatch::onReset()
{
    if (!view_ || peaks_.isEmpty()) {
        if (needFit_)
            needFit_();
        return;
    }
    pendingRecheck_ = true;
    if (needFit_)
        needFit_();
}
