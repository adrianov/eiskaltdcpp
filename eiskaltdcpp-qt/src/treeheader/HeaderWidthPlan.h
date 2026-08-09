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
#include <QVector>

class QAbstractItemView;
class QHeaderView;

/**
 * Measured soft/full column widths and how they are applied to a header:
 * set floors, bring side columns to p100 before the widest, then give that
 * column any leftover; shrink modest overflow evenly.
 */
class HeaderWidthPlan
{
public:
    HeaderWidthPlan(QAbstractItemView *view, const QSet<int> &manual);

    bool measure(QHeaderView *header);
    int sumSoft() const { return sumSoft_; }
    int sumFull() const { return sumFull_; }

    void setSoft(QHeaderView *header) const;
    void setFull(QHeaderView *header) const;
    void fillSpare(QHeaderView *header, int room) const;
    /** Cut the widest column toward its title floor by up to `over` pixels. */
    void trimWidest(QHeaderView *header, int over) const;
    void shrinkOverflow(QHeaderView *header, int viewW) const;

private:
    struct Col {
        int col = 0;
        int soft = 0;
        int full = 0;
        int floor = 0;
    };

    int widest() const;
    void scale(QHeaderView *header, const QVector<int> &cols,
               const QVector<int> &sizes, int total, int budget) const;

    QAbstractItemView *view_ = nullptr;
    QSet<int> manual_;
    QVector<Col> autos_;
    int sumSoft_ = 0;
    int sumFull_ = 0;
};
