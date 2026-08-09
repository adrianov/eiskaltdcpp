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

/**
 * Defers header column fitting until the view is ready. One content fit runs
 * after rows exist; later inserts do not keep rewriting widths.
 * Columns the user has dragged stay at that width (no further autosize).
 * Does not watch ancestor widgets — dock/window drags must stay fluid.
 */
class TreeHeaderAutosize : public QObject
{
public:
    static void restore(QHeaderView *header, const QByteArray &state);
    static void ensure(QAbstractItemView *view);
    /** Logical column that grows/shrinks to fit the viewport; -1 = first visible. */
    static void setStretchColumn(QAbstractItemView *view, int logicalColumn);

private:
    explicit TreeHeaderAutosize(QAbstractItemView *view);

    static TreeHeaderAutosize *attached(QAbstractItemView *view);
    void hookHeader();
    void requestFit();
    void hookModel();
    void scheduleCheck();
    void checkLayout();
    bool eventFilter(QObject *obj, QEvent *ev) override;

    QPointer<QAbstractItemView> view_;
    int stretchColumn_ = -1;
    bool done_ = false;
    bool pending_ = false;
    bool modelHooked_ = false;
    bool contentFit_ = false; // one content fit after the model has rows
    bool fitting_ = false;    // ignore sectionResized from our own fits
    QSet<int> manual_;
};
