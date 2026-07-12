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

} // namespace

/** Replace the former flat share_entries DB with an empty normalized schema.
    A full row rewrite of multi-GB indexes blocks the write worker (and thus
    search stats / list ingest) for hours; cached file lists re-ingest.

    If share_locations already exists (normalized DB), only drop a leftover
    share_entries table — never wipe an already-migrated index. An older
    SQLite build can recreate an empty share_entries beside DuckDB tables. */
bool ShareIndex::compactLegacyDb()
{
    duckdb::Connection *con = threadConn();
    if (!con)
        return false;
    if (!hasTable(*con, "share_entries"))
        return true;

    if (hasTable(*con, "share_locations")) {
        QString err;
        if (!ShareIndexDb::execOk(*con, "DROP TABLE IF EXISTS share_entries", &err)) {
            setLastError(err);
            return false;
        }
        return true;
    }

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
