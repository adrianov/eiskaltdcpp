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

const qint64 kMaxEntries = 5000000;

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
    auto res = con.Query(
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_name = 'share_index_meta' LIMIT 1");
    return !res->HasError() && res->RowCount() > 0;
}

} // namespace

void ShareIndex::refreshEntryCount(duckdb::Connection &con)
{
    auto res = con.Query("SELECT count(*)::BIGINT FROM share_entries");
    if (res->HasError() || res->RowCount() == 0) {
        setLastError(QString::fromStdString(res->GetError()));
        return;
    }
    QString err;
    if (!upsertMeta(con, QStringLiteral("entry_count"),
                    ShareIndexDb::qi64(res->GetValue(0, 0)), &err))
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

    if (metaValue(con, QStringLiteral("entry_count")) < 0) {
        QString err;
        auto res = con.Query("SELECT count(*)::BIGINT FROM share_entries");
        if (res->HasError() || res->RowCount() == 0) {
            setLastError(QString::fromStdString(res->GetError()));
            return false;
        }
        if (!upsertMeta(con, QStringLiteral("entry_count"),
                        ShareIndexDb::qi64(res->GetValue(0, 0)), &err)) {
            setLastError(err);
            return false;
        }
    }
    return true;
}

void ShareIndex::pruneExcess(duckdb::Connection &con)
{
    const qint64 count = metaValue(con, QStringLiteral("entry_count"), 0);
    const qint64 excess = count - kMaxEntries;
    if (excess <= 0)
        return;

    auto res = ShareIndexDb::query1(
        con,
        "DELETE FROM share_entries WHERE id IN ("
        "SELECT id FROM share_entries WHERE source = 2 "
        "ORDER BY created_at ASC LIMIT ?)",
        ShareIndexDb::i64Val(excess));
    if (!res || res->HasError()) {
        setLastError(res ? QString::fromStdString(res->GetError()) : QStringLiteral("prune"));
        return;
    }
    refreshEntryCount(con);
}

#endif
