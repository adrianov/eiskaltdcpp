/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QStyleOptionProgressBar>

#include "AppTheme.h"

namespace {

inline QColor progressFillColor(const QStyleOptionViewItem &option, const QPalette *barPalette)
{
    if (barPalette) {
        const QColor custom = barPalette->color(QPalette::Highlight);
        if (custom.isValid())
            return custom;
    }

#if defined(Q_OS_MAC)
    return AppTheme::linkColor();
#endif
    return option.palette.color(QPalette::Highlight);
}

inline QColor progressTextColor(const QStyleOptionViewItem &option)
{
    if (option.state & QStyle::State_Selected)
        return option.palette.color(QPalette::HighlightedText);

    return option.palette.color(QPalette::Text);
}

inline QColor progressTrackColor(const QStyleOptionViewItem &option)
{
    const bool selected = option.state & QStyle::State_Selected;
    const bool dark = AppTheme::isDark();

    if (selected)
        return dark ? QColor(255, 255, 255, 48) : QColor(255, 255, 255, 72);

    return dark ? QColor(255, 255, 255, 36) : QColor(0, 0, 0, 24);
}

inline QColor progressBorderColor()
{
    return AppTheme::isDark() ? QColor(255, 255, 255, 54) : QColor(0, 0, 0, 42);
}

inline QColor readableFillColor(const QColor &fill, const QColor &track)
{
    if (qAbs(fill.lightness() - track.lightness()) >= 45)
        return fill;

    return qAbs(AppTheme::linkColor().lightness() - track.lightness()) >= 45
        ? AppTheme::linkColor()
        : AppTheme::successColor();
}

inline void paintProgressText(QPainter *painter, const QRect &cell,
                              const QStyleOptionViewItem &option, const QString &text)
{
    if (text.isEmpty())
        return;

    const QColor shadow = AppTheme::isDark() ? QColor(0, 0, 0, 110) : QColor(255, 255, 255, 140);
    painter->setPen(shadow);
    painter->drawText(cell.adjusted(0, 1, 0, 1), Qt::AlignCenter, text);
    painter->setPen(progressTextColor(option));
    painter->drawText(cell, Qt::AlignCenter, text);
}

inline void paintBarChunk(QPainter *painter, const QRect &barRect, int percent, const QColor &fill)
{
    const int bounded = qBound(0, percent, 100);
    if (bounded <= 0)
        return;

    QRect chunk = barRect;
    const qreal radius = qMin(barRect.height() / 2.0, 5.0);
    chunk.setWidth(qMax(static_cast<int>(radius * 2), barRect.width() * bounded / 100));
    painter->setBrush(fill);
    painter->drawRoundedRect(chunk, radius, radius);
}

inline void paintStyledProgressBar(QPainter *painter, const QStyleOptionViewItem &option,
                                   int percent, const QString &text, const QPalette *barPalette)
{
    const QRect cell = option.rect;

    if (option.state & QStyle::State_Selected)
        painter->fillRect(cell, option.palette.highlight());

    const QRect barRect = cell.adjusted(3, 4, -3, -4);
    const qreal radius = qMin(barRect.height() / 2.0, 5.0);
    const QColor track = progressTrackColor(option);
    const QColor fill = readableFillColor(progressFillColor(option, barPalette), track);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(progressBorderColor());
    painter->setBrush(track);
    painter->drawRoundedRect(barRect, radius, radius);
    painter->setPen(Qt::NoPen);
    paintBarChunk(painter, barRect, percent, fill);
    paintProgressText(painter, cell, option, text);
    painter->restore();
}

} // namespace

// Native item-delegate progress bars can lose contrast in themed views.
inline void paintProgressCell(QPainter *painter, const QStyleOptionViewItem &option,
                              int percent, const QString &text, const QPalette *barPalette = nullptr)
{
#if defined(USE_PROGRESS_BARS)
    paintStyledProgressBar(painter, option, percent, text, barPalette);
#else
    QStyleOptionViewItem plain = option;
    plain.text = text;
    plain.displayAlignment = Qt::AlignCenter;
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &plain, painter);
#endif
}
