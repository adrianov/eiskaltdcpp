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
    // Do not open() here — schema open runs on the write worker.
    if (!isOpen())
        return stats;

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

bool ShareIndex::needsListIngest(const QString &cid, const QString &listPath)
{
    if (cid.isEmpty())
        return false;

    // Not open yet → allow enqueue; write worker opens first.
    if (!isOpen())
        return true;

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return false;

    if (listPath.isEmpty()) {
        QSqlQuery q(db);
        q.prepare("SELECT 1 FROM share_entries WHERE cid = ? LIMIT 1");
        q.addBindValue(cid);
        if (!q.exec())
            return false;
        return !q.next();
    }

    const QFileInfo fi(listPath);
    if (!fi.exists())
        return false;

    QSqlQuery q(db);
    q.prepare("SELECT list_path, list_mtime, list_size FROM share_list_meta WHERE cid = ?");
    q.addBindValue(cid);
    if (!q.exec() || !q.next()) {
        // Pre-meta DB: keep existing rows searchable; record current file stamp.
        QSqlQuery has(db);
        has.prepare("SELECT 1 FROM share_entries WHERE cid = ? AND source = ? LIMIT 1");
        has.addBindValue(cid);
        has.addBindValue(int(SourceFileList));
        if (has.exec() && has.next()) {
            rememberListMeta(cid, listPath, 0);
            return false;
        }
        return true;
    }

    return q.value(0).toString() != listPath
            || q.value(1).toLongLong() != fi.lastModified().toSecsSinceEpoch()
            || q.value(2).toLongLong() != fi.size();
}

void ShareIndex::rememberListMeta(const QString &cid, const QString &listPath, int rowCount)
{
    if (cid.isEmpty() || listPath.isEmpty())
        return;

    const QFileInfo fi(listPath);
    if (!fi.exists())
        return;

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return;

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO share_list_meta(cid, list_path, list_mtime, list_size, row_count, indexed_at) "
        "VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(cid) DO UPDATE SET "
        "list_path=excluded.list_path, list_mtime=excluded.list_mtime, "
        "list_size=excluded.list_size, row_count=excluded.row_count, "
        "indexed_at=excluded.indexed_at");
    q.addBindValue(cid);
    q.addBindValue(listPath);
    q.addBindValue(fi.lastModified().toSecsSinceEpoch());
    q.addBindValue(fi.size());
    q.addBindValue(rowCount);
    q.addBindValue(nowStamp());
    if (!q.exec())
        setLastError(q.lastError().text());
}

#endif
