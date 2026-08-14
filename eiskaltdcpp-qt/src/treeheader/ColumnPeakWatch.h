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

#pragma once

#include "treeheader/ColumnContentSpan.h"

#include <QHash>
#include <QSet>
#include <functional>

class QAbstractItemView;
class QModelIndex;

/**
 * Tracks per-column peak content widths. needFit runs when a peak grows.
 * On reset or row removal, a peak shrinks only if the new longest is at
 * least 30% shorter; smaller drops are skipped.
 */
class ColumnPeakWatch
{
public:
    using NeedFit = std::function<void()>;

    explicit ColumnPeakWatch(QAbstractItemView *view);

    void setManual(const QSet<int> *manual);
    void setNeedFit(NeedFit fn);
    const QHash<int, ColumnWidths> &peaks() const { return peaks_; }
    void mergePeaks(const QHash<int, ColumnWidths> &measured);
    void clearPeaks();
    void recheck();

    void onInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &tl, const QModelIndex &br,
                       const QVector<int> &roles);
    void onReset();

private:
    bool noteWider(int col, int width);
    void fitIfGrew(bool grew);

    QAbstractItemView *view_ = nullptr;
    const QSet<int> *manual_ = nullptr;
    NeedFit needFit_;
    QHash<int, ColumnWidths> peaks_;
    bool pendingRecheck_ = false;
};
