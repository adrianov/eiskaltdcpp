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

#include <QFileInfo>
#include <QSqlQuery>

ShareIndex::IndexStats ShareIndex::indexStats()
{
    IndexStats stats;
    open();
    if (!opened)
        return stats;

    QMutexLocker lock(&mutex);
    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return stats;

    QSqlQuery q(db);
    if (q.exec("SELECT count(*) FROM share_entries WHERE is_dir = 0") && q.next())
        stats.files = q.value(0).toLongLong();

    const QFileInfo dbInfo(dbFile);
    if (dbInfo.exists())
        stats.dbBytes = dbInfo.size();

    const QFileInfo wal(dbFile + QStringLiteral("-wal"));
    if (wal.exists())
        stats.dbBytes += wal.size();

    const QFileInfo shm(dbFile + QStringLiteral("-shm"));
    if (shm.exists())
        stats.dbBytes += shm.size();

    return stats;
}

bool ShareIndex::needsListIngest(const QString &cid)
{
    if (cid.isEmpty())
        return false;

    open();
    if (!opened)
        return false;

    QMutexLocker lock(&mutex);
    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM share_entries WHERE cid = ? LIMIT 1");
    q.addBindValue(cid);
    if (!q.exec())
        return false;
    return !q.next();
}

#endif
