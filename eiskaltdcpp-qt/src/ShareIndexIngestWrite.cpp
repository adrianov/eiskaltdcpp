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

#include "ShareIndexQueueCore.h"

using namespace ShareIndexWriteQueue;

bool ShareIndex::writeListRows(const QString &cid, const QList<QVariantMap> &rows)
{
    duckdb::Connection *con = threadConn();
    if (!con) {
        setLastError(QStringLiteral("threadConn not open"));
        return false;
    }
    if (!ShareIndexDb::execOk(*con, "BEGIN TRANSACTION")) {
        setLastError(QStringLiteral("BEGIN failed"));
        return false;
    }
    auto del = ShareIndexDb::query1(
        *con, "DELETE FROM share_locations WHERE user_id IN ("
        "SELECT user_id FROM share_users WHERE cid = ?)", ShareIndexDb::strVal(cid));
    if (!del || del->HasError()) {
        ShareIndexDb::execOk(*con, "ROLLBACK");
        setLastError(del ? QString::fromStdString(del->GetError()) : QStringLiteral("delete"));
        return false;
    }

    const int chunk = 20000;
    for (int offset = 0; offset < rows.size(); ) {
        if (isStopping()) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return false;
        }
        const int end = qMin(offset + chunk, rows.size());
        QList<QVariantMap> slice;
        slice.reserve(end - offset);
        for (; offset < end; ++offset)
            slice.append(rows.at(offset));

        if (!appendListRows(*con, slice)) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return false;
        }
    }

    if (!ShareIndexDb::execOk(*con, "COMMIT")) {
        setLastError(QStringLiteral("COMMIT failed"));
        return false;
    }
    if (!removeOrphans(*con))
        return false;
    refreshEntryCount(*con);
    pruneExcess(*con);
    reclaimFreePages(*con);
    return true;
}

void ShareIndex::removeTthSync(const QString &cid, const QString &tth)
{
    if (cid.isEmpty() || tth.isEmpty() || isStopping())
        return;
    duckdb::Connection *con = threadConn();
    if (!con) {
        setLastError(QStringLiteral("threadConn not open"));
        return;
    }
    QString err;
    auto res = ShareIndexDb::query2(
        *con,
        "DELETE FROM share_locations WHERE user_id IN ("
        "SELECT user_id FROM share_users WHERE cid = ?) AND file_id IN ("
        "SELECT file_id FROM share_files WHERE tth = ?)",
        ShareIndexDb::strVal(cid), ShareIndexDb::strVal(tth), &err);
    if (!res || res->HasError()) {
        setLastError(err.isEmpty() && res ? QString::fromStdString(res->GetError()) : err);
        return;
    }
    if (!removeOrphans(*con))
        return;
    refreshEntryCount(*con);
}

void ShareIndex::removeUserSync(const QString &cid)
{
    if (cid.isEmpty() || isStopping())
        return;
    duckdb::Connection *con = threadConn();
    if (!con) {
        setLastError(QStringLiteral("threadConn not open"));
        return;
    }
    QString err;
    auto locs = ShareIndexDb::query1(
        *con,
        "DELETE FROM share_locations WHERE user_id IN ("
        "SELECT user_id FROM share_users WHERE cid = ?)",
        ShareIndexDb::strVal(cid), &err);
    if (!locs || locs->HasError()) {
        setLastError(err.isEmpty() && locs ? QString::fromStdString(locs->GetError()) : err);
        return;
    }
    ShareIndexDb::query1(*con, "DELETE FROM share_list_meta WHERE cid = ?",
                         ShareIndexDb::strVal(cid));
    if (!removeOrphans(*con))
        return;
    refreshEntryCount(*con);
}

bool ShareIndex::removeOrphans(duckdb::Connection &con)
{
    QString err;
    if (!ShareIndexDb::execOk(con,
            "DELETE FROM share_files f WHERE NOT EXISTS ("
            "SELECT 1 FROM share_locations l WHERE l.file_id = f.file_id)",
            &err)) {
        setLastError(err.isEmpty() ? QStringLiteral("orphan files") : err);
        return false;
    }
    if (!ShareIndexDb::execOk(con,
            "DELETE FROM share_users u WHERE NOT EXISTS ("
            "SELECT 1 FROM share_locations l WHERE l.user_id = u.user_id)",
            &err)) {
        setLastError(err.isEmpty() ? QStringLiteral("orphan users") : err);
        return false;
    }
    return true;
}

#endif
