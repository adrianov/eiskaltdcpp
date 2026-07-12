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
class QPalette;
class QProgressBar;
class QRectF;
class QWidget;

/** Palette helpers and SwiftUI-like control chrome on top of Qt 5 (Fusion). */
class AppTheme {
public:
    static bool isDark();
    static bool isDark(const QPalette &palette);
    static void apply();
    /** Prefer Fusion when no style is saved — flat, consistent controls. */
    static void applyPreferredStyle();

    static qreal controlRadius();
    static QColor inputBackground();
    static QColor inputBorder(bool focused);

    /** Common palette derived once and shared by every themed input control. */
    struct InputColors { QColor bg, text, muted, selBg, selFg; };
    static InputColors inputColors();
    static QColor readableColor(const QColor &bg, const QColor &preferred);

    /** App-wide combo / checkbox / radio rules; covers widgets created later. */
    static QString comboStyleSheet();
    static void applyInputPalette(QWidget *widget);
    static void applyControlButton(QAbstractButton *button);
    static void applyProgressBar(QProgressBar *bar);
    static void paintControlBorder(QPainter *painter, const QRectF &rect, bool focused);

    static QString chatColor(const QString &settingKey);
    /** Prefer preferred when it contrasts with the chat background; else black/white. */
    static QColor readableChatColor(const QColor &preferred);
    static QColor errorColor();
    static QColor successColor();
    static QColor linkColor();
    static QColor sharedFileColor();
    /** Soft row tint for files already in the local share (search / file browser). */
    static QColor sharedFileHighlight();
    static QColor findHighlightColor();
    static QColor chatBackground();
    /** Chat Base color: palette or custom setting when enabled. */
    static QColor effectiveChatBackground();
    /**
     * Restore contrast for baked-in chat HTML after a light/dark switch
     * (QEvent::PaletteChange / ThemeChange). See AppThemeChatDoc.cpp.
     */
    static void refreshChatViews();
    static bool isLegacyDefault(const QString &colorName);
    static bool isLegacyBackground(const QColor &color);
};
