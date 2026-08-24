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

#include "filebrowser/AdultCueSet.h"

namespace AdultVideo {

namespace {

/** DC share lists use backslash separators even on Unix. */
QString joinPath(const QString &path, const QString &name)
{
    if (path.isEmpty())
        return name;
    if (path.endsWith(QLatin1Char('\\')) || path.endsWith(QLatin1Char('/')))
        return path + name;
    return path + QLatin1Char('\\') + name;
}

} // namespace

bool matches(const QString &name, const QString &path)
{
    const QString hay = joinPath(path, name);
    if (hay.isEmpty())
        return false;
    return AdultCueSet::instance().matches(hay);
}

} // namespace AdultVideo
