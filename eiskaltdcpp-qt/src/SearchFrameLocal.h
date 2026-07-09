/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QVariantMap>
#include <QStringList>

class SearchFrame;

/** Local ShareIndex query + hub-result upsert helpers for SearchFrame. */
namespace SearchFrameLocal {

void startLocalSearch(SearchFrame *frame, const QStringList &terms, bool isHash,
                      bool dirsOnly, bool filesOnly, qint64 size, int sizeMode,
                      const QStringList &extensions = QStringList());

void upsertHubResult(const QVariantMap &map);

} // namespace SearchFrameLocal
