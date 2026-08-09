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

namespace {

/** Hub hits accumulate; file-list rows are rewritten per user. */
const qint64 kMaxHubEntries = 5000000;
/** Oldest-first delete page under the open-time 1 GB memory_limit. */
const qint64 kPruneBatch = 10000;

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

bool pruneFail(duckdb::unique_ptr<duckdb::MaterializedQueryResult> &res, QString *err)
{
    if (res && !res->HasError())
        return false;
    if (err && err->isEmpty())
        *err = res ? QString::fromStdString(res->GetError()) : QStringLiteral("prune");
    return true;
}

/** Evict oldest hub-search locations in batches (memory_limit-safe). */
bool pruneHubLocations(duckdb::Connection &con, QString *err)
{
    for (;;) {
        qint64 n = 0;
        if (!ShareIndexDb::scalarI64(
                con, "SELECT count(*)::BIGINT FROM share_locations WHERE source = 2", &n, err))
            return false;
        const qint64 excess = n - kMaxHubEntries;
        if (excess <= 0)
            return true;
        const qint64 batch = excess < kPruneBatch ? excess : kPruneBatch;
        auto res = ShareIndexDb::query1(
            con,
            "DELETE FROM share_locations WHERE rowid IN ("
            "SELECT rowid FROM share_locations WHERE source = 2 "
            "ORDER BY created_at ASC, rowid ASC LIMIT ?)",
            ShareIndexDb::i64Val(batch), err);
        if (pruneFail(res, err))
            return false;
    }
}

} // namespace

bool ShareIndex::refreshEntryCount(duckdb::Connection &con)
{
    QString err;
    qint64 n = 0;
    // HUD "files indexed" = unique TTH rows (share_files), not location fan-out.
    if (!ShareIndexDb::scalarI64(con, "SELECT count(*)::BIGINT FROM share_files", &n, &err)
            || !upsertMeta(con, QStringLiteral("file_count"), n, &err)) {
        setLastError(err);
        return false;
    }
    return true;
}

bool ShareIndex::ensureCap(duckdb::Connection &con)
{
    QString err;
    if (!ShareIndexDb::execOk(con,
            "CREATE TABLE IF NOT EXISTS share_index_meta ("
            "key TEXT PRIMARY KEY,"
            "value BIGINT NOT NULL DEFAULT 0)", &err)) {
        setLastError(err);
        return false;
    }

    // Missing file_count: first open after upgrade from location-based entry_count.
    if (metaValue(con, QStringLiteral("file_count")) < 0 && !refreshEntryCount(con))
        return false;
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

bool ShareIndex::pruneExcess(duckdb::Connection &con)
{
    QString err;
    qint64 hubCount = 0;
    if (!ShareIndexDb::scalarI64(
            con, "SELECT count(*)::BIGINT FROM share_locations WHERE source = 2",
            &hubCount, &err)) {
        setLastError(err);
        return false;
    }
    if (hubCount <= kMaxHubEntries)
        return true;
    // Orphan share_files cleanup stays on write paths (too heavy for open).
    if (!pruneHubLocations(con, &err)) {
        setLastError(err.isEmpty() ? QStringLiteral("prune locations") : err);
        return false;
    }
    return true;
}

#endif
