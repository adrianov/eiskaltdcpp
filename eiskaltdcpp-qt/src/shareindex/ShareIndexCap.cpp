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

/** Unique TTH rows in share_files; over this, open wipes and rebuilds empty. */
const qint64 kMaxShareFiles = 5000000;

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

bool ShareIndex::filesOverCap(duckdb::Connection &con)
{
    // ensureCap always materializes file_count before this runs.
    return metaValue(con, QStringLiteral("file_count"), 0) > kMaxShareFiles;
}

#endif
