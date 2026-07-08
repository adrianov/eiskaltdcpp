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

inline QColor progressTrackColor(const QStyleOptionViewItem &option)
{
    const bool selected = option.state & QStyle::State_Selected;
    const bool dark = AppTheme::isDark();

    if (selected)
        return dark ? QColor(255, 255, 255, 48) : QColor(255, 255, 255, 72);

    return dark ? QColor(255, 255, 255, 36) : QColor(0, 0, 0, 24);
}

#if defined(Q_OS_MAC)
inline void paintMacProgressBar(QPainter *painter, const QStyleOptionViewItem &option,
                                int percent, const QString &text, const QPalette *barPalette)
{
    const QRect cell = option.rect;

    if (option.state & QStyle::State_Selected)
        painter->fillRect(cell, option.palette.highlight());

    const QRect barRect = cell.adjusted(3, 4, -3, -4);
    const qreal radius = qMin(barRect.height() / 2.0, 5.0);
    const QColor fill = progressFillColor(option, barPalette);
    const QColor track = progressTrackColor(option);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(track);
    painter->drawRoundedRect(barRect, radius, radius);

    const int bounded = qBound(0, percent, 100);
    if (bounded > 0) {
        QRect chunk = barRect;
        chunk.setWidth(qMax(static_cast<int>(radius * 2), barRect.width() * bounded / 100));
        painter->setBrush(fill);
        painter->drawRoundedRect(chunk, radius, radius);
    }
    painter->restore();

    if (text.isEmpty())
        return;

    painter->save();
    QColor textColor = option.palette.color(QPalette::Text);
    if (option.state & QStyle::State_Selected)
        textColor = option.palette.color(QPalette::HighlightedText);

    const QColor shadow = AppTheme::isDark() ? QColor(0, 0, 0, 110) : QColor(255, 255, 255, 140);
    painter->setPen(shadow);
    painter->drawText(cell.adjusted(0, 1, 0, 1), Qt::AlignCenter, text);
    painter->setPen(textColor);
    painter->drawText(cell, Qt::AlignCenter, text);
    painter->restore();
}
#endif

} // namespace

// macOS Qt styles omit progress bar chrome and overlay text in item delegates.
inline void paintProgressCell(QPainter *painter, const QStyleOptionViewItem &option,
                              int percent, const QString &text, const QPalette *barPalette = nullptr)
{
#if defined(USE_PROGRESS_BARS)
#if defined(Q_OS_MAC)
    paintMacProgressBar(painter, option, percent, text, barPalette);
#else
    QStyleOptionProgressBar bar;
    bar.state = QStyle::State_Enabled;
    bar.direction = QApplication::layoutDirection();
    bar.rect = option.rect;
    bar.fontMetrics = QApplication::fontMetrics();
    bar.minimum = 0;
    bar.maximum = 100;
    bar.textAlignment = Qt::AlignCenter;
    bar.textVisible = true;
    bar.text = text;
    bar.progress = qBound(0, percent, 100);
    bar.palette = barPalette ? *barPalette : option.palette;

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &bar, painter);
#endif
#else
    QStyleOptionViewItem plain = option;
    plain.text = text;
    plain.displayAlignment = Qt::AlignCenter;
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &plain, painter);
#endif
}
