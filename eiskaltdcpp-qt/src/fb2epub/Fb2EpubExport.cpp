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

#include "Fb2EpubExport.h"

#include "Fb2EpubConverter.h"
#include "Fb2Format.h"
#include "WulforUtil.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace Fb2EpubExport {

bool isFb2Name(const QString &pathOrName)
{
    return HomeCompa::Util::IsFb2Path(pathOrName);
}

bool isFb2File(const QString &path)
{
    return isFb2Name(path) && QFileInfo::exists(path);
}

QStringList existingFb2Files(const QStringList &paths)
{
    QStringList out;
    for (const auto &path : paths) {
        if (isFb2File(path))
            out.push_back(path);
    }
    return out;
}

int convertAndReveal(const QStringList &paths)
{
    int ok = 0;
    for (const auto &fb2Path : existingFb2Files(paths)) {
        const QString epubPath = HomeCompa::Util::EpubPathForFb2(fb2Path);
        if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
            continue;
        WulforUtil::revealPath(epubPath);
        ++ok;
    }
    return ok;
}

bool convertIsDefaultOpen()
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

void activateFiles(const QStringList &paths)
{
    QStringList fb2;
    for (const auto &path : paths) {
        if (path.isEmpty())
            continue;
        if (convertIsDefaultOpen() && isFb2Name(path))
            fb2.append(path);
        else
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
    if (!fb2.isEmpty())
        convertAndReveal(fb2);
}

} // namespace Fb2EpubExport
