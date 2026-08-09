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
 * Content-based header widths: soft (p80+30%) or p100 when all p100 fit;
 * enlarge non-manual columns to fill free space; if total is at most 30%
 * over the viewport, scale every column down to fit.
 */
class HeaderContentFit
{
public:
    HeaderContentFit(QAbstractItemView *view, const QSet<int> &manual);

    static QHeaderView *headerOf(QAbstractItemView *view);

    bool ready() const;
    void apply();

private:
    void scaleToViewport(QHeaderView *header, int viewW) const;

    QAbstractItemView *view_ = nullptr;
    QSet<int> manual_;
};
