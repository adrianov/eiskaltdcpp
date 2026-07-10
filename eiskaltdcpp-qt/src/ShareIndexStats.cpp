/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

#include <QDateTime>
#include <QFileInfo>

ShareIndex::IndexStats ShareIndex::indexStats()
{
    IndexStats stats;
    if (!isOpen())
        return stats;

    duckdb::Connection *con = threadConn();
    if (!con)
        return stats;

    auto res = con->Query("SELECT value FROM share_index_meta WHERE key = 'entry_count'");
    if (!res->HasError() && res->RowCount() > 0)
        stats.files = ShareIndexDb::qi64(res->GetValue(0, 0));

    const QFileInfo dbInfo(dbFile);
    if (dbInfo.exists())
        stats.dbBytes = dbInfo.size();

    const QFileInfo wal(dbFile + QStringLiteral(".wal"));
    if (wal.exists())
        stats.dbBytes += wal.size();

    return stats;
}

bool ShareIndex::needsListIngest(const QString &cid, const QString &listPath)
{
    if (cid.isEmpty())
        return false;

    if (!isOpen())
        return true;

    duckdb::Connection *con = threadConn();
    if (!con)
        return true;

    if (listPath.isEmpty()) {
        auto res = ShareIndexDb::query1(*con,
            "SELECT 1 FROM share_locations l JOIN share_users u USING(user_id) "
            "WHERE u.cid = ? LIMIT 1",
                                        ShareIndexDb::strVal(cid));
        if (!res || res->HasError())
            return true;
        return res->RowCount() == 0;
    }

    const QFileInfo fi(listPath);
    if (!fi.exists())
        return false;

    auto res = ShareIndexDb::query1(
        *con, "SELECT list_path, list_mtime, list_size FROM share_list_meta WHERE cid = ?",
        ShareIndexDb::strVal(cid));
    if (!res || res->HasError() || res->RowCount() == 0) {
        // No meta → ingest (including after a partial/aborted write).
        return true;
    }

    return ShareIndexDb::qstr(res->GetValue(0, 0)) != listPath
            || ShareIndexDb::qi64(res->GetValue(1, 0)) != fi.lastModified().toSecsSinceEpoch()
            || ShareIndexDb::qi64(res->GetValue(2, 0)) != fi.size();
}

void ShareIndex::rememberListMeta(const QString &cid, const QString &listPath, int rowCount)
{
    if (cid.isEmpty() || listPath.isEmpty())
        return;
    if (ShareIndexWriteQueue::isStopping())
        return;

    const QFileInfo fi(listPath);
    if (!fi.exists())
        return;

    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    duckdb::vector<duckdb::Value> binds;
    binds.push_back(ShareIndexDb::strVal(cid));
    binds.push_back(ShareIndexDb::strVal(listPath));
    binds.push_back(ShareIndexDb::i64Val(fi.lastModified().toSecsSinceEpoch()));
    binds.push_back(ShareIndexDb::i64Val(fi.size()));
    binds.push_back(ShareIndexDb::i64Val(rowCount));
    binds.push_back(ShareIndexDb::strVal(nowStamp()));
    QString err;
    auto res = ShareIndexDb::queryMat(
        *con,
        "INSERT INTO share_list_meta(cid, list_path, list_mtime, list_size, row_count, indexed_at) "
        "VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(cid) DO UPDATE SET "
        "list_path=excluded.list_path, list_mtime=excluded.list_mtime, "
        "list_size=excluded.list_size, row_count=excluded.row_count, "
        "indexed_at=excluded.indexed_at",
        binds, &err);
    if (!res)
        setLastError(err);
}

#endif
