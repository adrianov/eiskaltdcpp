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

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

#include "ShareIndexQueueCore.h"

#include <limits>

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
    // Drop the file row only when no location still references that TTH.
    ShareIndexDb::query1(
        *con,
        "DELETE FROM share_files WHERE tth = ? AND NOT EXISTS ("
        "SELECT 1 FROM share_locations l WHERE l.file_id = share_files.file_id)",
        ShareIndexDb::strVal(tth));
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
    // Drop the user row(s) even if file-orphan batches fail later.
    ShareIndexDb::query1(*con, "DELETE FROM share_users WHERE cid = ?",
                         ShareIndexDb::strVal(cid));
    if (!removeOrphans(*con))
        return;
    refreshEntryCount(*con);
}

namespace {

/** Page size for PK orphan deletes under memory_limit. */
const qint64 kOrphanPage = 1000;

/** Walk parent PKs in pages; delete only unreferenced ids in each page.
 *  A full ANTI JOIN still hashes every row and has FatalException'd on commit. */
bool orphanSweep(duckdb::Connection &con, const char *table, const char *idCol, QString *err)
{
    qint64 cursor = std::numeric_limits<qint64>::min();
    const std::string id = idCol;
    const std::string tbl = table;
    for (;;) {
        if (ShareIndexWriteQueue::isStopping())
            return false;
        duckdb::vector<duckdb::Value> binds;
        binds.push_back(ShareIndexDb::i64Val(cursor));
        binds.push_back(ShareIndexDb::i64Val(kOrphanPage));
        auto page = ShareIndexDb::queryMat(
            con,
            "SELECT " + id + " FROM " + tbl + " WHERE " + id + " > ? ORDER BY " + id
                + " LIMIT ?",
            binds, err);
        if (!page || page->HasError()) {
            if (err && err->isEmpty())
                *err = page ? QString::fromStdString(page->GetError())
                            : QStringLiteral("orphan page");
            return false;
        }
        const idx_t n = page->RowCount();
        if (n == 0)
            return true;
        const qint64 last = ShareIndexDb::qi64(page->GetValue(0, n - 1));
        auto del = ShareIndexDb::query2(
            con,
            "DELETE FROM " + tbl + " WHERE " + id + " IN ("
            "SELECT t." + id + " FROM " + tbl + " t WHERE t." + id + " > ? AND t." + id
                + " <= ? AND NOT EXISTS ("
                "SELECT 1 FROM share_locations l WHERE l." + id + " = t." + id + "))",
            ShareIndexDb::i64Val(cursor), ShareIndexDb::i64Val(last), err);
        if (!del || del->HasError()) {
            if (err && err->isEmpty())
                *err = del ? QString::fromStdString(del->GetError())
                           : QStringLiteral("orphan delete");
            return false;
        }
        cursor = last;
        if (n < idx_t(kOrphanPage))
            return true;
    }
}

} // namespace

bool ShareIndex::removeOrphans(duckdb::Connection &con)
{
    QString err;
    // Location indexes speed NOT EXISTS; drop cid index before user deletes so a
    // torn secondary ART cannot invalidate the whole DB.
    ShareIndexDb::execOk(con, "DROP INDEX IF EXISTS share_users_cid");
    ShareIndexDb::execOk(con,
            "CREATE INDEX IF NOT EXISTS share_loc_file ON share_locations(file_id)");
    ShareIndexDb::execOk(con,
            "CREATE INDEX IF NOT EXISTS share_loc_user ON share_locations(user_id)");
    if (!orphanSweep(con, "share_files", "file_id", &err)
            || !orphanSweep(con, "share_users", "user_id", &err)) {
        setLastError(err.isEmpty() ? QStringLiteral("orphan cleanup") : err);
        return false;
    }
    ShareIndexDb::execOk(con,
            "CREATE INDEX IF NOT EXISTS share_users_cid ON share_users(cid)");
    return true;
}

#endif
