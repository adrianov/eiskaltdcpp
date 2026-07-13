/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QString>

/** Resolve a search-result TTH to a local file (own share or finished download). */
namespace SearchLocalPath {

/** Real path if the TTH is shared or a complete finished download still exists. */
QString resolve(const QString &tth, qint64 size);

/** True when an unfinished non-filelist queue item has this TTH. */
bool isQueued(const QString &tth);

bool openFile(const QString &path);
bool openDirectory(const QString &path);

} // namespace SearchLocalPath
