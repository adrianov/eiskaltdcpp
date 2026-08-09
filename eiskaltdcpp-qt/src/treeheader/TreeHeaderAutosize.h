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

class QAbstractItemView;
class QEvent;
class QHeaderView;
class QTimer;

/**
 * Debounced column autosize for tree/table headers:
 * - width = max(header title, p80 or p100 if within 20% of p80); scroll OK
 * - user-dragged columns stay fixed; cell value updates alone do not refit
 * - refit when the row count changes (1s debounce after the last change)
 * - saved header state is kept on load until a later row-count fit
 */
class TreeHeaderAutosize : public QObject
{
public:
    static void restore(QHeaderView *header, const QByteArray &state);
    static void ensure(QAbstractItemView *view);

private:
    explicit TreeHeaderAutosize(QAbstractItemView *view);

    static TreeHeaderAutosize *attached(QAbstractItemView *view);
    static QHeaderView *headerOf(QAbstractItemView *view);

    void hookHeader();
    void requestFit();
    void hookModel();
    void scheduleCheck();
    void checkLayout();
    void applyFit();
    bool eventFilter(QObject *obj, QEvent *ev) override;

    QPointer<QAbstractItemView> view_;
    QTimer *debounce_ = nullptr;
    bool done_ = false;
    bool modelHooked_ = false;
    bool fitting_ = false;
    QSet<int> manual_;
};
