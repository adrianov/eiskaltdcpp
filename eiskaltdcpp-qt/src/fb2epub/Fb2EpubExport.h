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

#pragma once

#include <QString>
#include <QStringList>

/** UI helper: turn local FB2/FBD files into EPUB next to the source. */
namespace Fb2EpubExport {

/** True when the path or file name ends with .fb2 / .fbd (no disk check). */
bool isFb2Name(const QString &pathOrName);
/** True when path is an existing FB2/FBD file. */
bool isFb2File(const QString &path);
QStringList existingFb2Files(const QStringList &paths);
/** Convert each existing FB2; reveal each EPUB. Returns success count. */
int convertAndReveal(const QStringList &paths);

/** On macOS, FB2/FBD default activation is Convert to EPUB (not Open). */
bool convertIsDefaultOpen();
/** Open each path; on macOS FB2/FBD files are converted and revealed instead. */
void activateFiles(const QStringList &paths);

} // namespace Fb2EpubExport
