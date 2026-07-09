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

#include <QFile>
#include <QSqlQuery>

namespace {

/** 16 KiB: fewer I/Os on multi-GB indexes; set only on empty DBs. */
const int kPageSize = 16384;
/** Truncate ~4 MiB per incremental_vacuum call (stall bound). */
const int kVacuumChunk = 256;
/** Cap reclaim per write (~32 MiB); leftover freelist drains later. */
const int kVacuumMaxPages = 2048;

bool dbHasUserTables(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "SELECT 1 FROM sqlite_master WHERE type='table' "
            "AND name NOT LIKE 'sqlite_%' LIMIT 1")))
        return true;
    return q.next();
}

} // namespace

bool ShareIndex::ensureAutoVacuum(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("PRAGMA auto_vacuum")) || !q.next())
        return false;
    const int vacuum = q.value(0).toInt(); // 0=none, 1=full, 2=incremental

    int pageSize = 0;
    if (q.exec(QStringLiteral("PRAGMA page_size")) && q.next())
        pageSize = q.value(0).toInt();

    if (vacuum == 2 && pageSize >= kPageSize)
        return true;

    // page_size / auto_vacuum need an empty DB (else full VACUUM = 2× disk).
    // Keep existing indexes; do not delete user data to force the setting.
    if (dbHasUserTables(db))
        return true;

    if (pageSize < kPageSize
            && !q.exec(QStringLiteral("PRAGMA page_size=%1").arg(kPageSize)))
        return false;
    if (vacuum != 2
            && !q.exec(QStringLiteral("PRAGMA auto_vacuum=INCREMENTAL")))
        return false;
    return true;
}

bool ShareIndex::recreateForVacuum()
{
    if (dbFile.isEmpty())
        return false;

    {
        QSqlDatabase db = threadDb();
        if (db.isOpen() && dbHasUserTables(db))
            return ensureAutoVacuum(db);
    }

    disconnectThreadDb();
    QFile::remove(dbFile);
    QFile::remove(dbFile + QStringLiteral("-wal"));
    QFile::remove(dbFile + QStringLiteral("-shm"));

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return false;
    return ensureAutoVacuum(db);
}

void ShareIndex::reclaimFreePages(QSqlDatabase &db)
{
    QSqlQuery q(db);
    int left = kVacuumMaxPages;
    while (left > 0) {
        if (!q.exec(QStringLiteral("PRAGMA freelist_count")) || !q.next())
            return;
        const qint64 free = q.value(0).toLongLong();
        if (free <= 0)
            break;

        const int n = qMin(kVacuumChunk, left);
        // No-op unless auto_vacuum=incremental; never runs a full copy VACUUM.
        q.exec(QStringLiteral("PRAGMA incremental_vacuum(%1)").arg(n));
        left -= n;
    }
    // PASSIVE never blocks readers; skip if another connection holds a lock.
    q.exec(QStringLiteral("PRAGMA wal_checkpoint(PASSIVE)"));
}

#endif
