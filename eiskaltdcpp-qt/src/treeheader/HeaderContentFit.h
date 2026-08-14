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

class QAbstractItemView;
class QHeaderView;

/**
 * Content-sizes columns (p100 when it fits, else soft), then proportionally
 * enlarges non-manual columns into spare width. Does not shrink to fill.
 * Known peaks are a floor and skip a fresh content walk.
 */
class HeaderContentFit
{
public:
    HeaderContentFit(QAbstractItemView *view, const QSet<int> &manual);

    static QHeaderView *headerOf(QAbstractItemView *view);

    bool ready() const;
    QHash<int, ColumnWidths> apply(const QHash<int, ColumnWidths> &peaks);

private:
    QAbstractItemView *view_ = nullptr;
    QSet<int> manual_;
};
