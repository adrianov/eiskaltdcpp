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

/** File/folder list icons: OS type icons when available, else bundled map. */

#include "WulforUtil.h"

#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QMimeDatabase>

namespace {

constexpr int kListSide = 16;

QFileIconProvider &iconProvider()
{
    static QFileIconProvider provider;
    return provider;
}

QString cacheKey(const QString &ext)
{
    return (ext.isEmpty() ? QStringLiteral(".") : ext)
            + QLatin1Char('@')
            + QString::number(AppIcons::deviceRatio(), 'f', 2);
}

/**
 * QFileIconProvider on macOS/Qt5 returns dpr=1 bitmaps. Request the physical
 * pixel size we paint (16 * deviceRatio), then tag dpr — avoids soft upscales.
 */
QPixmap pixmapFromIcon(const QIcon &icon)
{
    if (icon.isNull())
        return QPixmap();

    const qreal dpr = AppIcons::deviceRatio();
    const int pixelSide = qMax(1, qRound(kListSide * dpr));
    QPixmap pm = icon.pixmap(pixelSide, pixelSide);
    if (pm.isNull())
        return QPixmap();

    if (pm.width() == pixelSide && pm.height() == pixelSide) {
        pm.setDevicePixelRatio(dpr);
        return pm;
    }
    return AppIcons::scale(pm, kListSide);
}

QPixmap systemFilePixmap(const QFileInfo &info, const QString &ext)
{
    QIcon icon;
    if (info.isAbsolute() && info.exists())
        icon = iconProvider().icon(info);
    else {
        const QString probe = ext.isEmpty()
                ? QStringLiteral("file")
                : QStringLiteral("file.") + ext.toLower();
        icon = iconProvider().icon(QFileInfo(probe));
    }

    QPixmap pm = pixmapFromIcon(icon);
    if (!pm.isNull())
        return pm;

    static QMimeDatabase db;
    const QFileInfo mimeInfo = info.isAbsolute()
            ? info
            : QFileInfo(ext.isEmpty() ? QStringLiteral("file")
                                      : QStringLiteral("file.") + ext.toLower());
    const QMimeType mime = db.mimeTypeForFile(mimeInfo, QMimeDatabase::MatchExtension);
    if (!mime.isValid())
        return QPixmap();
    icon = QIcon::fromTheme(mime.iconName());
    if (icon.isNull())
        icon = QIcon::fromTheme(mime.genericIconName());
    return pixmapFromIcon(icon);
}

} // namespace

const QPixmap &WulforUtil::getPixmapForFile(const QString &file)
{
    const QFileInfo info(file);
    const QString ext = info.suffix().toUpper();
    const QString key = cacheKey(ext);

    const auto cached = m_SystemFileIconCache.constFind(key);
    if (cached != m_SystemFileIconCache.constEnd())
        return cached.value();

    const QPixmap system = systemFilePixmap(info, ext);
    if (!system.isNull())
        return m_SystemFileIconCache.insert(key, system).value();

    if (m_FileTypeMap.contains(ext))
        return getPixmap(m_FileTypeMap[ext]);
    return getPixmap(AppIcons::eiFILETYPE_UNKNOWN);
}

const QPixmap &WulforUtil::getPixmapForFolder()
{
    const QString key = cacheKey(QStringLiteral("/"));
    const auto cached = m_SystemFileIconCache.constFind(key);
    if (cached != m_SystemFileIconCache.constEnd())
        return cached.value();

    const QPixmap system = pixmapFromIcon(iconProvider().icon(QFileIconProvider::Folder));
    if (!system.isNull())
        return m_SystemFileIconCache.insert(key, system).value();
    return getPixmap(AppIcons::eiFOLDER_BLUE);
}
