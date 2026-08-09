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

#include <QSet>

class QAbstractItemView;
class QHeaderView;

/**
 * Applies HeaderWidthPlan to a visible tree/table: p100 when it fits,
 * otherwise soft floors, side columns grow to p100 first, widest takes
 * leftover. Modest soft overflow (≤30%) scales every column down evenly.
 */
class HeaderContentFit
{
public:
    HeaderContentFit(QAbstractItemView *view, const QSet<int> &manual);

    static QHeaderView *headerOf(QAbstractItemView *view);

    bool ready() const;
    void apply();

private:
    QAbstractItemView *view_ = nullptr;
    QSet<int> manual_;
};
