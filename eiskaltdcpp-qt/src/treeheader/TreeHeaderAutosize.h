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
#include <QList>
#include <QObject>
#include <QPointer>

class QAbstractItemView;
class QEvent;
class QHeaderView;

/**
 * Fits a tree/table header to content and viewport width.
 * Optionally one logical column absorbs leftover or deficit width (fit-to-width).
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
    static QHeaderView *viewHeader(QAbstractItemView *view);

    bool eventFilter(QObject *obj, QEvent *ev) override;
    void hookModel();
    void checkLayout();
    void autosizeColumns();
    bool layoutOk() const;
    QList<int> visibleColumns() const;
    int headerLabelWidth(int column) const;
    int columnContentWidth(int column) const;
    int stretchIndex(const QList<int> &visible) const;

    QPointer<QAbstractItemView> view_;
    int stretchColumn_ = -1;
    bool done_ = false;
    bool modelHooked_ = false;
};
