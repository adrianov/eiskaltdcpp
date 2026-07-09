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

#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QResource>
#if QT_VERSION >= 0x050000
#include <QGuiApplication>
#include <QScreen>
#endif

#include "icons/gv.xpm"

static const int PXMTHEMESIDE = THEME_ICON_SIZE;

qreal WulforUtil::iconDeviceRatio()
{
#if QT_VERSION >= 0x050000
    qreal dpr = 1.0;
    const auto screens = QGuiApplication::screens();
    for (const QScreen *screen : screens) {
        if (screen)
            dpr = qMax(dpr, screen->devicePixelRatio());
    }
    return dpr;
#else
    return 1.0;
#endif
}

// Render a pixmap at the physical resolution of the screen so icons stay sharp on Retina.
QPixmap WulforUtil::scalePixmap(const QPixmap &source, int logicalSide)
{
    if (source.isNull() || logicalSide <= 0)
        return source;

    const qreal dpr = iconDeviceRatio();
    const int pixelSide = qMax(1, qRound(logicalSide * dpr));

#if QT_VERSION >= 0x050000
    if (source.width() == pixelSide && source.height() == pixelSide
            && qFuzzyCompare(source.devicePixelRatio(), dpr))
        return source;
#else
    if (source.width() == pixelSide && source.height() == pixelSide)
        return source;
#endif

    QPixmap result = QPixmap::fromImage(
        source.toImage().scaled(pixelSide, pixelSide, Qt::KeepAspectRatio, Qt::SmoothTransformation));
#if QT_VERSION >= 0x050000
    result.setDevicePixelRatio(dpr);
#endif
    return result;
}

QPixmap WulforUtil::FROMTHEME(const QString &name, bool resource){
    const QPixmap source = resource ? QPixmap(":/" + name + ".png") : loadPixmap(name + ".png");
    return scalePixmap(source, PXMTHEMESIDE);
}

QPixmap WulforUtil::FROMTHEME_SIDE(const QString &name, bool resource, const int side){
    const QPixmap source = resource ? QPixmap(":/" + name + ".png") : loadPixmap(name + ".png");
    return scalePixmap(source, side);
}

