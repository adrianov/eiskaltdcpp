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
#include "treeheader/ColumnContentSpan.h"
#include "treeheader/HeaderContentFit.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHeaderView>

namespace {
constexpr int kSampleRows = 300; // ColumnContentSpan
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

void ColumnPeakWatch::setPeaks(QHash<int, int> peaks)
{
    peaks_ = std::move(peaks);
}

void ColumnPeakWatch::clearPeaks()
{
    peaks_.clear();
}

bool ColumnPeakWatch::noteWider(int col, int width)
{
    if (width < 1 || (manual_ && manual_->contains(col)))
        return false;
    if (peaks_.isEmpty() || width > peaks_.value(col, 0)) {
        if (needFit_)
            needFit_();
        return true;
    }
    return false;
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
    for (int r = first; r <= end; ++r) {
        for (int v = 0; v < header->count(); ++v) {
            const int col = header->logicalIndex(v);
            if (header->isSectionHidden(col) || (manual_ && manual_->contains(col)))
                continue;
            if (noteWider(col, span.cellWidth(model->index(r, col, parent))))
                return;
        }
    }
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
    for (int r = tl.row(); r <= end; ++r) {
        for (int c = tl.column(); c <= br.column(); ++c) {
            const QModelIndex idx = tl.sibling(r, c);
            if (!idx.isValid() || (manual_ && manual_->contains(c)))
                continue;
            if (noteWider(c, span.cellWidth(idx)))
                return;
        }
    }
}
