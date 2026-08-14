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

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QSet>

#include <memory>

class ColumnPeakWatch;
class QAbstractItemView;
class QEvent;
class QHeaderView;
class QTimer;

/**
 * Column autosize: short debounce on show/resize, plus ColumnPeakWatch when
 * a column's peak content width grows. Later shorter values shrink a column
 * only if the new longest is at least 30% shorter. Sort is not hooked.
 */
class TreeHeaderAutosize : public QObject
{
public:
    static void restore(QHeaderView *header, const QByteArray &state);
    static void ensure(QAbstractItemView *view);

private:
    explicit TreeHeaderAutosize(QAbstractItemView *view);

    static TreeHeaderAutosize *attached(QAbstractItemView *view);

    void hookHeader();
    void hookModel();
    void requestFit();
    void scheduleCheck();
    void checkLayout();
    bool eventFilter(QObject *obj, QEvent *ev) override;

    QPointer<QAbstractItemView> view_;
    QTimer *debounce_ = nullptr;
    std::unique_ptr<ColumnPeakWatch> peaks_;
    bool done_ = false;
    bool modelHooked_ = false;
    bool fitting_ = false;
    QSet<int> manual_;
};
