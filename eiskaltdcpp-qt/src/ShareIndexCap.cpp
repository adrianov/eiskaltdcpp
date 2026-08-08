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

namespace {

const qint64 kMaxHubEntries = 5000000;

bool upsertMeta(duckdb::Connection &con, const QString &key, qint64 value, QString *err)
{
    auto res = ShareIndexDb::query2(
        con,
        "INSERT INTO share_index_meta(key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
        ShareIndexDb::strVal(key), ShareIndexDb::i64Val(value), err);
    return res && !res->HasError();
}

qint64 metaValue(duckdb::Connection &con, const QString &key, qint64 fallback = -1)
{
    QString err;
    auto res = ShareIndexDb::query1(con, "SELECT value FROM share_index_meta WHERE key = ?",
                                    ShareIndexDb::strVal(key), &err);
    if (!res || res->HasError() || res->RowCount() == 0)
        return fallback;
    return ShareIndexDb::qi64(res->GetValue(0, 0));
}

bool metaTableExists(duckdb::Connection &con)
{
    qint64 one = 0;
    return ShareIndexDb::scalarI64(con,
            "SELECT 1 FROM information_schema.tables "
            "WHERE table_name = 'share_index_meta' LIMIT 1",
            &one);
}

} // namespace

void ShareIndex::refreshEntryCount(duckdb::Connection &con)
{
    QString err;
    qint64 n = 0;
    if (!ShareIndexDb::scalarI64(con, "SELECT count(*)::BIGINT FROM share_locations", &n, &err)) {
        setLastError(err);
        return;
    }
    if (!upsertMeta(con, QStringLiteral("entry_count"), n, &err))
        setLastError(err);
}

bool ShareIndex::ensureCap(duckdb::Connection &con)
{
    if (!metaTableExists(con)) {
        QString err;
        if (!ShareIndexDb::execOk(con,
                "CREATE TABLE share_index_meta ("
                "key TEXT PRIMARY KEY,"
                "value BIGINT NOT NULL DEFAULT 0)", &err)) {
            setLastError(err);
            return false;
        }
    }

    // Recount when missing or stuck at 0 (stale meta after a partial migrate).
    if (metaValue(con, QStringLiteral("entry_count")) <= 0) {
        QString err;
        qint64 n = 0;
        if (!ShareIndexDb::scalarI64(con, "SELECT count(*)::BIGINT FROM share_locations",
                                     &n, &err)) {
            setLastError(err);
            return false;
        }
        if (!upsertMeta(con, QStringLiteral("entry_count"), n, &err)) {
            setLastError(err);
            return false;
        }
    }
    QString err;
    if (!upsertMeta(con, QStringLiteral("schema_files_tth"), 1, &err)) {
        setLastError(err);
        return false;
    }
    // One-shot: empty media was stored as 0/'' — store NULL instead.
    if (metaValue(con, QStringLiteral("media_null"), 0) < 1) {
        ShareIndexDb::execOk(con, "UPDATE share_files SET bitrate=NULL WHERE bitrate=0");
        ShareIndexDb::execOk(con, "UPDATE share_files SET resolution=NULL WHERE resolution=''");
        ShareIndexDb::execOk(con, "UPDATE share_files SET video=NULL WHERE video=''");
        ShareIndexDb::execOk(con, "UPDATE share_files SET audio=NULL WHERE audio=''");
        if (!upsertMeta(con, QStringLiteral("media_null"), 1, &err)) {
            setLastError(err);
            return false;
        }
    }
    return true;
}

void ShareIndex::pruneExcess(duckdb::Connection &con)
{
    // File-list rows mirror lists cached on disk and are replaced per user;
    // only hub-search rows grow without bound, so the cap applies to them.
    qint64 hubCount = 0;
    if (!ShareIndexDb::scalarI64(con,
            "SELECT count(*)::BIGINT FROM share_locations WHERE source = 2", &hubCount))
        return;
    const qint64 excess = hubCount - kMaxHubEntries;
    if (excess <= 0)
        return;

    auto res = ShareIndexDb::query1(
        con,
        "DELETE FROM share_locations WHERE rowid IN ("
        "SELECT rowid FROM share_locations WHERE source = 2 "
        "ORDER BY created_at ASC LIMIT ?)",
        ShareIndexDb::i64Val(excess));
    if (!res || res->HasError()) {
        setLastError(res ? QString::fromStdString(res->GetError()) : QStringLiteral("prune"));
        return;
    }
    if (!removeOrphans(con))
        return;
    refreshEntryCount(con);
}

#endif
