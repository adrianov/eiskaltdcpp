/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppTheme.h"
#include "WulforSettings.h"

#include <QApplication>
#include <QPalette>
#include <QSet>

/** Minimum lightness gap so chat text stays readable (white-on-white, etc.). */
static const int kChatContrast = 50;

static QColor paletteText(){
    return qApp->palette().color(QPalette::Text);
}

static QColor effectiveChatBackground(){
    QColor clr = AppTheme::chatBackground();
    if (!WulforSettings::getInstance()->getBool("hubframe/change-chat-background-color", false))
        return clr;

    QColor custom;
    custom.setNamedColor(WulforSettings::getInstance()->getStr("hubframe/chat-background-color"));
    if (custom.isValid() && !AppTheme::isLegacyBackground(custom))
        return custom;
    return clr;
}

static bool hasChatContrast(const QColor &fg, const QColor &bg = QColor()){
    if (!fg.isValid())
        return false;
    const QColor against = bg.isValid() ? bg : effectiveChatBackground();
    return qAbs(fg.lightness() - against.lightness()) >= kChatContrast;
}

static QColor paletteSecondary(){
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    const QColor placeholder = qApp->palette().color(QPalette::PlaceholderText);
    if (hasChatContrast(placeholder))
        return placeholder;
#endif
    if (AppTheme::isDark())
        return paletteText().darker(130);

    // Readable muted gray on light chat backgrounds (not washed-out PlaceholderText).
    return QColor(0x6C, 0x6C, 0x70);
}

QColor AppTheme::readableChatColor(const QColor &preferred){
    const QColor bg = effectiveChatBackground();
    if (hasChatContrast(preferred, bg))
        return preferred;
    return readableColor(bg, preferred);
}

bool AppTheme::isLegacyDefault(const QString &colorName){
    static const QSet<QString> legacy = {
        "#078010", "#000000", "#838383", "#ffff00", "#ac0000",
        "#2344e7", "#ff0000", "#00ff7f", "#1f8f1f"
    };
    return legacy.contains(colorName.toLower());
}

bool AppTheme::isLegacyBackground(const QColor &color){
    if (!color.isValid())
        return true;

    if (!isDark())
        return false;

    return color.lightness() > 200;
}

QColor AppTheme::errorColor(){
    return isDark() ? QColor(0xFF, 0x45, 0x3A) : QColor(0xFF, 0x3B, 0x30);
}

QColor AppTheme::successColor(){
    return isDark() ? QColor(0x32, 0xD7, 0x4B) : QColor(0x34, 0xC7, 0x59);
}

QColor AppTheme::linkColor(){
    return isDark() ? QColor(0x0A, 0x84, 0xFF) : QColor(0x00, 0x7A, 0xFF);
}

QColor AppTheme::sharedFileColor(){
    return successColor();
}

QColor AppTheme::sharedFileHighlight(){
    QColor c;
    const QString stored = WulforSettings::getInstance()->getStr(WS_APP_SHARED_FILES_COLOR);
    if (stored.isEmpty() || isLegacyDefault(stored))
        c = successColor();
    else
        c.setNamedColor(stored);
    if (!c.isValid())
        c = successColor();

    // Soft wash keeps default text readable; strength comes from settings alpha.
    int alpha = WulforSettings::getInstance()->getInt(WI_APP_SHARED_FILES_ALPHA, 56);
    if (alpha < 0)
        alpha = 0;
    else if (alpha > 255)
        alpha = 255;
    c.setAlpha(alpha);
    return c;
}

QColor AppTheme::findHighlightColor(){
    return isDark() ? QColor(0xFF, 0xD6, 0x0A) : QColor(0xFF, 0xFF, 0x00);
}

QColor AppTheme::chatBackground(){
    return qApp->palette().color(QPalette::Base);
}

static QColor defaultChatColor(const QString &key){
    if (key == WS_CHAT_STAT_COLOR || key == WS_CHAT_CORE_COLOR)
        return AppTheme::errorColor();

    if (key == WS_CHAT_USER_COLOR || key == WS_CHAT_PRIV_USER_COLOR)
        return AppTheme::isDark() ? QColor(0xFF, 0x69, 0x6B) : QColor(0xC0, 0x00, 0x00);

    if (key == WS_CHAT_LOCAL_COLOR || key == WS_CHAT_PRIV_LOCAL_COLOR)
        return AppTheme::successColor();

    if (key == WS_CHAT_OP_COLOR)
        return AppTheme::isDark() ? QColor(0xFF, 0xD6, 0x0A) : QColor(0x00, 0x00, 0x00);

    if (key == WS_CHAT_BOT_COLOR)
        return paletteSecondary();

    if (key == WS_CHAT_SAY_NICK)
        return AppTheme::linkColor();

    if (key == WS_CHAT_FAVUSER_COLOR)
        return AppTheme::successColor();

    if (key == WS_CHAT_TIME_COLOR)
        return paletteSecondary();

    if (key == WS_CHAT_MSG_COLOR)
        return paletteText();

    if (key == WS_CHAT_FIND_COLOR)
        return AppTheme::findHighlightColor();

    if (key == WS_APP_SHARED_FILES_COLOR)
        return AppTheme::sharedFileColor();

    return paletteText();
}

QString AppTheme::chatColor(const QString &settingKey){
    // Highlight / list colors are backgrounds or non-chat surfaces — do not clamp.
    if (settingKey == WS_CHAT_FIND_COLOR || settingKey == WS_APP_SHARED_FILES_COLOR) {
        const QString stored = WulforSettings::getInstance()->getStr(settingKey);
        if (stored.isEmpty() || isLegacyDefault(stored))
            return defaultChatColor(settingKey).name();
        return stored;
    }

    if (settingKey == WS_CHAT_TIME_COLOR)
        return readableChatColor(paletteSecondary()).name();

    if (settingKey == WS_CHAT_MSG_COLOR)
        return readableChatColor(paletteText()).name();

    const QString stored = WulforSettings::getInstance()->getStr(settingKey);
    QColor color;
    if (stored.isEmpty() || isLegacyDefault(stored))
        color = defaultChatColor(settingKey);
    else {
        color.setNamedColor(stored);
        // Dark-theme leftovers (e.g. #ffffff op/bot) → theme default for this background.
        if (!hasChatContrast(color))
            color = defaultChatColor(settingKey);
    }

    return readableChatColor(color).name();
}
