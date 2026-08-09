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
 * How wide a tree/table column must be for its header title and cell text
 * (including icons). May briefly resize the section while probing.
 */
class ColumnContentSpan
{
public:
    explicit ColumnContentSpan(QAbstractItemView *view);

    int title(int column) const;
    int cells(int column) const;

private:
    QAbstractItemView *view_ = nullptr;
};
