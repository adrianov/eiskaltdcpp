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

#include "WulforUtil.h"
#include "WulforSettings.h"

#include <QFile>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QDir>
#include <QResource>
#include <QGuiApplication>
#include <QScreen>

#include "icons/gv.xpm"

static const int PXMTHEMESIDE = THEME_ICON_SIZE;

// Smooth path: premultiplied ARGB + progressive halving before the final scale.
// Much cleaner than one-shot downscale for large emoticon packs (e.g. 72→24).
// FastTransformation skips this so tiny pixel-art upscales stay crisp.
static QImage scaleImage(const QImage &source, int pixelSide, Qt::TransformationMode mode)
{
    if (mode != Qt::SmoothTransformation)
        return source.scaled(pixelSide, pixelSide, Qt::KeepAspectRatio, mode);

    QImage img = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QSize target = img.size().scaled(pixelSide, pixelSide, Qt::KeepAspectRatio);
    if (!target.isValid() || target.isEmpty())
        return img;

    while (img.width() >= target.width() * 2 && img.height() >= target.height() * 2)
        img = img.scaled(img.width() / 2, img.height() / 2,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if (img.size() == target)
        return img;
    return img.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

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

    QPixmap result = QPixmap::fromImage(scaleImage(source.toImage(), pixelSide, mode));
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
