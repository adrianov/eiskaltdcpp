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
#include <QGuiApplication>
#include <QScreen>

#include "icons/gv.xpm"

static const int PXMTHEMESIDE = THEME_ICON_SIZE;

qreal WulforUtil::iconDeviceRatio()
{
    qreal dpr = 1.0;
    const auto screens = QGuiApplication::screens();
    for (const QScreen *screen : screens) {
        if (screen)
            dpr = qMax(dpr, screen->devicePixelRatio());
    }
    return dpr;
}

// Render a pixmap at the physical resolution of the screen so icons stay sharp on Retina.
// Prefer SmoothTransformation when the source has enough pixels (theme icons, hi-res
// emoticon packs). FastTransformation remains available for tiny pixel-art upscales.
QPixmap WulforUtil::scalePixmap(const QPixmap &source, int logicalSide, Qt::TransformationMode mode)
{
    if (source.isNull() || logicalSide <= 0)
        return source;

    const qreal dpr = iconDeviceRatio();
    const int pixelSide = qMax(1, qRound(logicalSide * dpr));

    if (source.width() == pixelSide && source.height() == pixelSide
            && qFuzzyCompare(source.devicePixelRatio(), dpr))
        return source;

    QPixmap result = QPixmap::fromImage(
        source.toImage().scaled(pixelSide, pixelSide, Qt::KeepAspectRatio, mode));
    result.setDevicePixelRatio(dpr);
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

