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

#include <QPixmap>
#include <QString>

/**
 * Finder-style icon for a filename extension (no on-disk file required).
 * Call with a large pixelSide (e.g. 128); callers scale down for list rows.
 */
QPixmap macFileTypePixmap(const QString &ext, int pixelSide);
QPixmap macFolderPixmap(int pixelSide);
