/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"
#include "WulforSettings.h"

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

QString WulforUtil::findAppIconsPath() const
{
    // Try to find application icons directory
    const QString icon_theme = WSGET(WS_APP_ICONTHEME);

    QStringList settings_path_list = {
        QDir::currentPath() + "/icons/appl/" + icon_theme,
#if defined(Q_OS_MAC)
        bin_path + "/../Resources/icons/appl/" + icon_theme,
        "/Applications/EiskaltDC++.app/Contents/Resources/icons/appl/" + icon_theme,
#endif // defined(Q_OS_MAC)
        QDir::homePath() + "/.eiskaltdc++/icons/appl/" + icon_theme,
        bin_path + "/appl/" + icon_theme,
        CLIENT_ICONS_DIR "/appl/" + icon_theme,
        bin_path + CLIENT_ICONS_DIR "/appl/" + icon_theme,
        bin_path + "/../" CLIENT_ICONS_DIR "/appl/" + icon_theme,
        bin_path + "/../../" CLIENT_ICONS_DIR "/appl/" + icon_theme
    };

    for (QString settings_path : settings_path_list) {
        settings_path = QDir::toNativeSeparators(settings_path);
        if (QDir(settings_path).exists())
            return settings_path;
    }

    return QString();
}

QString WulforUtil::findUserIconsPath() const
{
    // Try to find icons directory
    const QString user_theme = WSGET(WS_APP_USERTHEME);

    QStringList settings_path_list = {
        QDir::currentPath() + "/icons/user/" + user_theme,
#if defined(Q_OS_MAC)
        bin_path + "/../Resources/icons/user/" + user_theme,
        "/Applications/EiskaltDC++.app/Contents/Resources/icons/user/" + user_theme,
#endif // defined(Q_OS_MAC)
        QDir::homePath() + "/.eiskaltdc++/icons/user/" + user_theme,
        bin_path + "icons/user/" + user_theme,
        bin_path + "/user/" + user_theme,
        CLIENT_ICONS_DIR "/user/" + user_theme,
        bin_path + CLIENT_ICONS_DIR "/user/" + user_theme,
        bin_path + "/../" CLIENT_ICONS_DIR "/user/" + user_theme,
        bin_path + "/../../" CLIENT_ICONS_DIR "/user/" + user_theme
    };

    for (QString settings_path : settings_path_list) {
        settings_path = QDir::toNativeSeparators(settings_path);
        if (QDir(settings_path).exists())
            return settings_path;
    }

    return QString();
}

QString WulforUtil::getAppIconsPath() const
{
    return app_icons_path;
}

QString WulforUtil::getEmoticonsPath() const
{
#if defined (Q_OS_WIN) || defined (Q_OS_HAIKU)
    static const QString emoticonsPath = bin_path + "/" CLIENT_DATA_DIR "/emoticons/";
#elif defined (Q_OS_MAC)
    static const QString emoticonsPath = bin_path + "/../Resources/emoticons/";
#else // Other OS
    static QString emoticonsPath = CLIENT_DATA_DIR "/emoticons/";
    if (!QDir(emoticonsPath).exists()) // Fix for Snap, AppImage, etc.
        emoticonsPath = bin_path + "/../../" + emoticonsPath;
#endif
    return QDir(emoticonsPath).absolutePath() + "/";
}

QString WulforUtil::getClientIconsPath() const
{
#if defined (Q_OS_WIN) || defined (Q_OS_HAIKU)
    static const QString iconsPath = bin_path + "/" CLIENT_ICONS_DIR "/";
#elif defined (Q_OS_MAC)
    static const QString iconsPath = bin_path + "/../Resources/" CLIENT_ICONS_DIR "/";
#else // Other OS
    static QString iconsPath = CLIENT_ICONS_DIR "/";
    if (!QDir(iconsPath).exists()) // Fix for Snap, AppImage, etc.
        iconsPath = QString(bin_path + "/../../" + iconsPath);
#endif
    return QDir(iconsPath).absolutePath();
}

QString WulforUtil::getTranslationsPath() const
{
#if defined (Q_OS_WIN) || defined (Q_OS_HAIKU)
    static const QString translationsPath = bin_path + "/" CLIENT_TRANSLATIONS_DIR "/";
#elif defined (Q_OS_MAC)
    static const QString translationsPath = bin_path + "/../Resources/translations/";
#else // Other OS
    static QString translationsPath = CLIENT_TRANSLATIONS_DIR "/";
    if (!QDir(translationsPath).exists()) // Fix for Snap, AppImage, etc.
        translationsPath = QString(bin_path + "/../../" + translationsPath);
#endif
    return QDir(QDir::toNativeSeparators(translationsPath)).absolutePath();
}

QString WulforUtil::getAspellDataPath() const
{
#if defined (Q_OS_WIN) || defined (Q_OS_HAIKU)
    static const QString aspellDataPath = bin_path + "/" CLIENT_DATA_DIR "/aspell/";
#elif defined (Q_OS_MAC)
    static const QString aspellDataPath = bin_path + "/../Resources/aspell/";
#elif defined(LOCAL_ASPELL_DATA) // Other OS
    static const QString aspellDataPath = CLIENT_DATA_DIR "/aspell/";
    if (!QDir(QDir::toNativeSeparators(aspellDataPath)).exists())
        return QString(bin_path + "/../../" + aspellDataPath);
#else
    static const QString aspellDataPath = QString();
#endif
    return QDir(aspellDataPath).absolutePath();
}

QString WulforUtil::getClientResourcesPath() const
{
    const QString icon_theme = WSGET(WS_APP_ICONTHEME);

#if defined(Q_OS_WIN) || defined(Q_OS_HAIKU)
    const QString client_res_path = bin_path + CLIENT_RES_DIR "/" + icon_theme + ".rcc";
#elif defined(Q_OS_MAC)
    const QString client_res_path = bin_path + QString("/../Resources/" CLIENT_RES_DIR "/") + icon_theme + ".rcc";
#else // Other systems
    QString client_res_path = QString(CLIENT_RES_DIR) + PATH_SEPARATOR_STR + icon_theme + ".rcc";
    if (!QDir(client_res_path).exists()) // Fix for Snap, AppImage, etc.
        client_res_path = bin_path + "/../../" + client_res_path;
#endif

    return QDir(client_res_path).absolutePath();;
}

