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
#include <QMutexLocker>

namespace {

bool hasTable(duckdb::Connection &con, const char *table)
{
    auto res = con.Query(std::string(
        "SELECT 1 FROM information_schema.tables WHERE table_catalog = current_database() "
        "AND table_schema = 'main' AND table_name = '")
        + table + "' LIMIT 1");
    return !res->HasError() && res->RowCount() > 0;
}

bool hasColumn(duckdb::Connection &con, const char *table, const char *column)
{
    auto res = con.Query(std::string(
        "SELECT 1 FROM information_schema.columns WHERE table_catalog = current_database() "
        "AND table_schema = 'main' AND table_name = '")
        + table + "' AND column_name = '" + column + "' LIMIT 1");
    return !res->HasError() && res->RowCount() > 0;
}

bool nullableColumn(duckdb::Connection &con, const char *table, const char *column)
{
    auto res = con.Query(std::string(
        "SELECT 1 FROM information_schema.columns WHERE table_catalog = current_database() "
        "AND table_schema = 'main' AND table_name = '")
        + table + "' AND column_name = '" + column
        + "' AND is_nullable = 'YES' LIMIT 1");
    return !res->HasError() && res->RowCount() > 0;
}

/** Current schema: canonical names and file identity by TTH alone. */
bool schemaCurrent(duckdb::Connection &con)
{
    if (!hasTable(con, "share_locations")
            || !hasColumn(con, "share_files", "name")
            || !hasColumn(con, "share_files", "path")
            || !hasColumn(con, "share_files", "ext")
            || !hasColumn(con, "share_files", "name_cf")
            || !hasColumn(con, "share_files", "path_cf")
            || !nullableColumn(con, "share_locations", "name")
            || !nullableColumn(con, "share_locations", "path")
            || !nullableColumn(con, "share_locations", "ext")
            || !nullableColumn(con, "share_locations", "name_cf")
            || !nullableColumn(con, "share_locations", "path_cf"))
        return false;
    if (!hasTable(con, "share_index_meta"))
        return false;
    auto res = ShareIndexDb::query1(
        con, "SELECT value FROM share_index_meta WHERE key=?",
        ShareIndexDb::strVal(QStringLiteral("schema_files_tth")));
    return res && !res->HasError() && res->RowCount() > 0
            && ShareIndexDb::qi64(res->GetValue(0, 0)) == 1;
}

} // namespace

/** Replace outdated DBs with an empty current schema. Full multi-GB rewrites
    would block the write worker; cached lists re-ingest instead.

    Handles flat share_entries, DBs without share_files.name, and composites
    UNIQUE(tth, size). Never wipe an already-current index — only drop a
    leftover share_entries. */
bool ShareIndex::compactLegacyDb()
{
    duckdb::Connection *con = threadConn();
    if (!con)
        return false;

    if (schemaCurrent(*con)) {
        if (!hasTable(*con, "share_entries"))
            return true;
        QString err;
        if (!ShareIndexDb::execOk(*con, "DROP TABLE IF EXISTS share_entries", &err)) {
            setLastError(err);
            return false;
        }
        return true;
    }

    const bool hasCore = hasTable(*con, "share_entries")
            || hasTable(*con, "share_locations")
            || hasTable(*con, "share_files")
            || hasTable(*con, "share_users");
    const bool hasMeta = hasTable(*con, "share_list_meta")
            || hasTable(*con, "share_index_meta");
    if (!hasCore && !hasMeta)
        return true;

    const QString newFile = dbFile + QStringLiteral(".compact");
    QFile::remove(newFile);
    QFile::remove(newFile + QStringLiteral(".wal"));

    QString attachFile = newFile;
    attachFile.replace(QLatin1Char('\''), QStringLiteral("''"));
    QString err;
    bool copied = ShareIndexDb::execOk(
                *con, "ATTACH '" + attachFile.toStdString() + "' AS compact", &err)
            && ensureSchema(*con, "compact.")
            && ShareIndexDb::execOk(*con, "DETACH compact", &err);

    {
        QMutexLocker lock(&connMutex);
        threadConns.clear();
    }
    duck.reset();

    const QString oldFile = dbFile + QStringLiteral(".migrate-old");
    QFile::remove(oldFile);
    bool swapped = false;
    if (copied && QFile::rename(dbFile, oldFile)) {
        swapped = QFile::rename(newFile, dbFile);
        if (!swapped)
            QFile::rename(oldFile, dbFile);
    }
    if (swapped) {
        QFile::remove(oldFile);
        QFile::remove(dbFile + QStringLiteral(".wal"));
    } else {
        setLastError(err.isEmpty() ? QStringLiteral("compact failed") : err);
        QFile::remove(newFile);
        QFile::remove(newFile + QStringLiteral(".wal"));
    }

    try {
        duck = std::make_unique<duckdb::DuckDB>(dbFile.toStdString());
    } catch (const std::exception &e) {
        setLastError(QString::fromUtf8(e.what()));
        duck.reset();
        return false;
    }

    con = threadConn();
    if (!con)
        return false;
    ShareIndexDb::execOk(*con, "SET memory_limit='1GB'");
    ShareIndexDb::execOk(*con, "SET threads=2");

    if (!swapped) {
        setLastError(err.isEmpty() ? QStringLiteral("share index migration failed") : err);
        return false;
    }
    return true;
}

#endif
