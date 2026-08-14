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

#include "filebrowser/AdultVideo.h"

#include <QRegularExpression>
#include <QString>

namespace AdultVideo {

namespace {

QString joinPath(const QString &path, const QString &name)
{
    if (path.isEmpty())
        return name;
    if (path.endsWith(QLatin1Char('\\')) || path.endsWith(QLatin1Char('/')))
        return path + name;
    return path + QLatin1Char('\\') + name;
}

bool hasKeyword(const QString &lower)
{
    static const char *const words[] = {
        "[18+]", "[adult]", "nsfw", "onlyfans", "porn", "xxx", "xvideos", "ladyboy"
    };
    for (const char *word : words) {
        if (lower.contains(QLatin1String(word)))
            return true;
    }
    return false;
}

bool hasJavCode(const QString &text)
{
    static const QRegularExpression jav(
            QStringLiteral(
                    "\\b(?:JUR|JUQ|JUL|ROE|ACHJ|SSIS|MIDE|IPX|FSDSS|MIMK|START|ABP|"
                    "STARS|MIDV|CAWD|HND|MEYD|WAAA|DASS|PPPE|SONE|FAD|SDDE|SDMU|RCT|"
                    "HEYZO|1PON|CARIB|10MU|FC2)-\\d{2,5}\\b"),
            QRegularExpression::CaseInsensitiveOption);
    return jav.match(text).hasMatch();
}

} // namespace

bool matches(const QString &name, const QString &path)
{
    const QString hay = joinPath(path, name);
    if (hay.isEmpty())
        return false;
    return hasKeyword(hay.toLower()) || hasJavCode(hay);
}

} // namespace AdultVideo
