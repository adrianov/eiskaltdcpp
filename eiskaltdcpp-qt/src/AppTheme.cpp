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

static QColor paletteText(){
    return qApp->palette().color(QPalette::Text);
}

static QColor paletteSecondary(){
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    const QColor placeholder = qApp->palette().color(QPalette::PlaceholderText);
    if (placeholder.isValid())
        return placeholder;
#endif
    QColor text = paletteText();
    return AppTheme::isDark() ? text.darker(130) : text.lighter(130);
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
        return paletteText();

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
    if (settingKey == WS_CHAT_TIME_COLOR)
        return paletteSecondary().name();

    if (settingKey == WS_CHAT_MSG_COLOR)
        return paletteText().name();

    const QString stored = WulforSettings::getInstance()->getStr(settingKey);
    if (stored.isEmpty() || isLegacyDefault(stored))
        return defaultChatColor(settingKey).name();

    return stored;
}
