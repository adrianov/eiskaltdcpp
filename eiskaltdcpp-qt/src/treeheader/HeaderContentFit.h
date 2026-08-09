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
 * Applies content-based header widths: grow columns that are too narrow;
 * shrink a column that is wider than its own content when there is scroll;
 * if total width is at most 30% over the viewport, scale every column
 * proportionally to fit.
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
