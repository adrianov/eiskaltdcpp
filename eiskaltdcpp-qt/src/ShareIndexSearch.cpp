/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

QList<QVariantMap> ShareIndex::search(const SearchFilter &filter)
{
    open();
    if (!isOpen() || filter.terms.isEmpty())
        return {};

    // No app-level lock: WAL + per-thread connections allow parallel searches.
    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return {};

    return searchFts(db, filter);
}

#endif
