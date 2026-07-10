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
bool shareIndexSmokeMigrate(const QString &path, QString *error);

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

    QVariantMap file;
    file["cid"] = "CIDTEST";
    file["hub_url"] = "adc://hub";
    file["tth"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    file["path"] = "TV\\Breaking Bad\\";
    file["name"] = "S01E01.mkv";
    file["is_dir"] = false;
    file["size"] = 12345;
    file["nick"] = "Alice";
    file["hub_name"] = "TestHub";
    if (!idx.upsertRow(*con, file, SourceFileList)) {
        if (error)
            *error = QString("insert file: %1").arg(idx.lastError());
        return false;
    }

    {
        auto extq = con->Query(
            "SELECT l.ext FROM share_locations l JOIN share_files f USING(file_id) "
            "WHERE f.tth != ''");
        if (extq->HasError() || extq->RowCount() == 0
                || ShareIndexDb::qstr(extq->GetValue(0, 0)) != QLatin1String("MKV")) {
            if (error)
                *error = "ext not MKV";
            return false;
        }
    }

    QVariantMap dirProp;
    dirProp["cid"] = "CIDTEST";
    dirProp["hub_url"] = "adc://hub";
    dirProp["tth"] = "";
    dirProp["path"] = "TV\\";
    dirProp["name"] = "Breaking Bad";
    dirProp["is_dir"] = true;
    dirProp["size"] = 0;
    dirProp["nick"] = "Alice";
    dirProp["hub_name"] = "TestHub";
    if (!idx.upsertRow(*con, dirProp, SourceFileList)) {
        if (error)
            *error = QString("insert dir: %1").arg(idx.lastError());
        return false;
    }

    con->Query(
        "UPDATE share_locations SET created_at='2000-01-01 00:00:00' "
        "WHERE file_id IS NOT NULL");

    file["name"] = "S01E01_renamed.mkv";
    file["size"] = 99999;
    if (!idx.upsertRow(*con, file, SourceHubSearch)) {
        if (error)
            *error = "upsert";
        return false;
    }

    auto created = con->Query(
        "SELECT created_at, name, source FROM share_locations WHERE file_id IS NOT NULL");
    if (created->HasError() || created->RowCount() == 0) {
        if (error)
            *error = "select after upsert";
        return false;
    }
    if (ShareIndexDb::qstr(created->GetValue(0, 0)) != QLatin1String("2000-01-01 00:00:00")) {
        if (error)
            *error = "created_at changed";
        return false;
    }
    if (ShareIndexDb::qstr(created->GetValue(1, 0)) != QLatin1String("S01E01_renamed.mkv")) {
        if (error)
            *error = "name not updated";
        return false;
    }
    if (ShareIndexDb::qi64(created->GetValue(2, 0)) != SourceFileList) {
        if (error)
            *error = "hub upsert demoted file_list source";
        return false;
    }

    if (!shareIndexSmokeSearch(idx, *con, error))
        return false;

    return shareIndexSmokeMigrate(dir.path() + "/legacy's.duckdb", error);
}

#endif
