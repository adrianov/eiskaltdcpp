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

#include <QTemporaryDir>

bool shareIndexSmokeSearch(ShareIndex &idx, duckdb::Connection &con, QString *error);
bool shareIndexSmokeUsers(ShareIndex &idx, duckdb::Connection &con, QString *error);
bool shareIndexSmokeMigrate(const QString &path, QString *error);
bool shareIndexSmokeWrites(ShareIndex &idx, duckdb::Connection &con, QString *error);

bool ShareIndex::smokeCheck(QString *error)
{
    QTemporaryDir dir;
    if (!dir.isValid()) {
        if (error)
            *error = "temp dir";
        return false;
    }

    ShareIndex idx;
    idx.dbFile = dir.path() + "/smoke.duckdb";
    try {
        idx.duck = std::make_unique<duckdb::DuckDB>(idx.dbFile.toStdString());
    } catch (const std::exception &e) {
        if (error)
            *error = QString::fromUtf8(e.what());
        return false;
    }

    duckdb::Connection *con = idx.threadConn();
    if (!con) {
        if (error)
            *error = "no connection";
        return false;
    }

    if (!idx.ensureSchema(*con) || !idx.ensureCap(*con)) {
        if (error)
            *error = idx.lastError().isEmpty() ? QStringLiteral("schema") : idx.lastError();
        return false;
    }
    idx.opened.storeRelease(1);

    if (!shareIndexSmokeWrites(idx, *con, error))
        return false;
    if (!shareIndexSmokeSearch(idx, *con, error))
        return false;
    if (!shareIndexSmokeUsers(idx, *con, error))
        return false;
    return shareIndexSmokeMigrate(dir.path() + "/legacy's.duckdb", error);
}

#endif
