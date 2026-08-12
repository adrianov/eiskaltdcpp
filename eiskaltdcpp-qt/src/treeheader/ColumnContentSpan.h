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

class QAbstractItemView;
class QModelIndex;

/** Soft (p80+30%) and full (p100) content widths, each at least the header title. */
struct ColumnWidths {
    int soft = 0;
    int full = 0;
};

/**
 * Column width from header title and cell text (with icons). Soft is
 * min(p80 + 30% of p80, p100); full is p100. Blank cells are not sampled.
 * Nested rows are walked via column 0 (hasChildren is invalid on other
 * columns). Column 0 includes tree indentation (and root decoration).
 */
class ColumnContentSpan
{
public:
    explicit ColumnContentSpan(QAbstractItemView *view);

    int title(int column) const;
    /** Single-cell width using the same rules as widths(). */
    int cellWidth(const QModelIndex &index) const;
    ColumnWidths widths(int column) const;

private:
    QAbstractItemView *view_ = nullptr;
};
