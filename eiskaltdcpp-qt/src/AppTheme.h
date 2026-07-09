/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QColor>
#include <QString>
#include <QtGlobal>

class QAbstractButton;
class QPainter;
class QRectF;
class QWidget;

/** Palette helpers and SwiftUI-like control chrome on top of Qt 5 (Fusion). */
class AppTheme {
public:
    static bool isDark();
    static void apply();
    /** Prefer Fusion when no style is saved — flat, consistent controls. */
    static void applyPreferredStyle();

    static qreal controlRadius();
    static QColor inputBackground();
    static QColor inputBorder(bool focused);
    static void applyInputPalette(QWidget *widget);
    static void applyControlButton(QAbstractButton *button);
    static void paintControlBorder(QPainter *painter, const QRectF &rect, bool focused);

    static QString chatColor(const QString &settingKey);
    static QColor errorColor();
    static QColor successColor();
    static QColor linkColor();
    static QColor sharedFileColor();
    static QColor findHighlightColor();
    static QColor chatBackground();
    static bool isLegacyDefault(const QString &colorName);
    static bool isLegacyBackground(const QColor &color);
};
