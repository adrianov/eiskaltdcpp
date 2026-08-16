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

#include <Qt>
#include <QtGlobal>
#include <QDropEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

namespace WulforQt {
constexpr auto SkipEmpty = Qt::SkipEmptyParts;
}

#define WULFOR_SKIP_EMPTY WulforQt::SkipEmpty

inline int wulforWheelDeltaY(QWheelEvent *e) {
    return e->angleDelta().y();
}

inline QPoint wulforEventPos(const QMouseEvent *e) {
    return e->position().toPoint();
}

inline QPoint wulforEventPos(const QDropEvent *e) {
    return e->position().toPoint();
}

inline void wulforSetMargin(QLayout *layout, int m) {
    if (layout)
        layout->setContentsMargins(m, m, m, m);
}

// Fails at compile time if WULFOR_SKIP_EMPTY is broken (e.g. self-referential macro).
inline QStringList wulforSplitSkipEmpty(const QString &s, QChar sep) {
    return s.split(sep, WULFOR_SKIP_EMPTY);
}

// Call from QSortFilterProxyModel subclasses only (begin/endFilterChange are protected).
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#define WULFOR_INVALIDATE_FILTER() do { beginFilterChange(); endFilterChange(); } while (0)
#else
#define WULFOR_INVALIDATE_FILTER() invalidateFilter()
#endif
