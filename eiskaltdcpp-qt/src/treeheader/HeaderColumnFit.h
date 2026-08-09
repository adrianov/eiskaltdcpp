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
#include <QSet>
#include <QVector>

class QAbstractItemView;
class QHeaderView;

/**
 * Lays out tree/table columns to content and the viewport. Keeps a section
 * when it is already wider than content; one flexible column absorbs leftover
 * space or shrinks on overflow. Columns in `manual` stay at the dragged width.
 */
class HeaderColumnFit
{
public:
    HeaderColumnFit(QAbstractItemView *view, int stretchColumn,
                    const QSet<int> &manual = QSet<int>());

    static QHeaderView *headerOf(QAbstractItemView *view);

    bool ready() const;
    bool fillsView() const;
    void apply();

private:
    QList<int> visibleColumns() const;
    int flexIndex(const QList<int> &visible) const;
    QVector<int> baseWidths(const QList<int> &visible, QHeaderView *header) const;
    void balance(QVector<int> &widths, const QList<int> &visible, int viewWidth) const;

    QAbstractItemView *view_ = nullptr;
    int stretchColumn_ = -1;
    QSet<int> manual_;
};
