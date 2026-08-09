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

/**
 * Column width from header title and cell text (with icons). Uses p80 of
 * non-empty sampled cells, or p100 when it is at most 20% wider than p80.
 */
class ColumnContentSpan
{
public:
    explicit ColumnContentSpan(QAbstractItemView *view);

    int title(int column) const;
    /** max(title, p80 or near p100); title alone when the column has no rows. */
    int cells(int column) const;

private:
    QAbstractItemView *view_ = nullptr;
};
