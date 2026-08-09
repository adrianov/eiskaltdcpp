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

#include <QHash>
#include <QSet>
#include <functional>

class QAbstractItemView;
class QModelIndex;

/**
 * Tracks per-column peak content widths. Calls needFit only when a peak
 * grows (narrower peaks are ignored).
 */
class ColumnPeakWatch
{
public:
    using NeedFit = std::function<void()>;

    explicit ColumnPeakWatch(QAbstractItemView *view);

    void setManual(const QSet<int> *manual);
    void setNeedFit(NeedFit fn);
    void setPeaks(QHash<int, int> peaks);
    void clearPeaks();

    void onInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &tl, const QModelIndex &br,
                       const QVector<int> &roles);

private:
    bool noteWider(int col, int width);

    QAbstractItemView *view_ = nullptr;
    const QSet<int> *manual_ = nullptr;
    NeedFit needFit_;
    QHash<int, int> peaks_;
};
