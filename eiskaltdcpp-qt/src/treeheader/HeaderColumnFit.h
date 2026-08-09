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

#include <QList>

class QAbstractItemView;
class QHeaderView;

/**
 * Measures tree/table column content and applies section widths so the header
 * fills the viewport. One logical column may absorb leftover or deficit width.
 */
class HeaderColumnFit
{
public:
    HeaderColumnFit(QAbstractItemView *view, int stretchColumn);

    static QHeaderView *headerOf(QAbstractItemView *view);

    bool canApply() const;
    bool isAdequate() const;
    void apply();

private:
    QList<int> visibleColumns() const;
    int labelWidth(int column) const;
    int contentWidth(int column) const;
    int stretchIndex(const QList<int> &visible) const;

    QAbstractItemView *view_ = nullptr;
    int stretchColumn_ = -1;
};
