/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
# include <Qt>
namespace WulforQt {
constexpr auto SkipEmpty = Qt::SkipEmptyParts;
}
#else
namespace WulforQt {
constexpr auto SkipEmpty = QString::SkipEmptyParts;
}
#endif

#define WULFOR_SKIP_EMPTY WulforQt::SkipEmpty

inline int wulforWheelDeltaY(QWheelEvent *e) {
#if QT_VERSION >= 0x050000
    return e->angleDelta().y();
#else
    return e->delta();
#endif
}

// Fails at compile time if WULFOR_SKIP_EMPTY is broken (e.g. self-referential macro).
inline QStringList wulforSplitSkipEmpty(const QString &s, QChar sep) {
    return s.split(sep, WULFOR_SKIP_EMPTY);
}
