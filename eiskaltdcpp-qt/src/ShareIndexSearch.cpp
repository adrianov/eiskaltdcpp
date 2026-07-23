/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

void ShareIndex::cancelSearch()
{
#ifdef USE_QT_SQLITE
    searchEpoch.fetchAndAddOrdered(1);
    QMutexLocker lock(&searchMu);
    if (activeSearchCon)
        activeSearchCon->Interrupt();
#endif
}

#ifdef USE_QT_SQLITE

QList<QVariantMap> ShareIndex::search(const SearchFilter &filter)
{
    if (!isOpen() || filter.terms.isEmpty())
        return {};

    duckdb::Connection *con = threadConn();
    if (!con)
        return {};

    // Interactive search: more RAM/threads than the write-path defaults.
    ShareIndexDb::execOk(*con, "SET memory_limit='4GB'");
    ShareIndexDb::execOk(*con, "SET threads=4");

    const int epoch = searchEpoch.loadAcquire();
    {
        QMutexLocker lock(&searchMu);
        if (activeSearchCon && activeSearchCon != con)
            activeSearchCon->Interrupt();
        activeSearchCon = con;
    }

    const QList<QVariantMap> rows = searchFts(*con, filter);

    {
        QMutexLocker lock(&searchMu);
        if (activeSearchCon == con)
            activeSearchCon = nullptr;
    }

    if (epoch != searchEpoch.loadAcquire())
        return {};

    return rows;
}

#endif
