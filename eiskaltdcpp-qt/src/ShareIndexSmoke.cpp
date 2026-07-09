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
#include <QUuid>

bool shareIndexSmokeSearch(ShareIndex &idx, QSqlDatabase &db, QString *error);

bool ShareIndex::smokeCheck(QString *error)
{
    QTemporaryDir dir;
    if (!dir.isValid()) {
        if (error)
            *error = "temp dir";
        return false;
    }

    const QString conn = "ShareIndexSmoke_" + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
        db.setDatabaseName(dir.path() + "/smoke.sqlite");
        if (!db.open()) {
            if (error)
                *error = db.lastError().text();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        ShareIndex idx;
        idx.dbFile = db.databaseName();
        if (!idx.ensureSchema(db) || !idx.ensureFts(db)) {
            if (error)
                *error = "schema";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

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
        if (!idx.upsertRow(db, file, SourceFileList)) {
            if (error)
                *error = QString("insert file: %1").arg(db.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        {
            QSqlQuery extq(db);
            extq.exec("SELECT ext FROM share_entries WHERE tth != ''");
            if (!extq.next() || extq.value(0).toString() != "MKV") {
                if (error)
                    *error = "ext not MKV";
                db.close();
                QSqlDatabase::removeDatabase(conn);
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
        if (!idx.upsertRow(db, dirProp, SourceFileList)) {
            if (error)
                *error = QString("insert dir: %1").arg(idx.lastError().isEmpty()
                                                           ? db.lastError().text()
                                                           : idx.lastError());
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        QSqlQuery touch(db);
        touch.exec("UPDATE share_entries SET created_at='2000-01-01 00:00:00', updated_at='2000-01-01 00:00:00' WHERE tth != ''");

        file["name"] = "S01E01_renamed.mkv";
        file["size"] = 99999;
        if (!idx.upsertRow(db, file, SourceHubSearch)) {
            if (error)
                *error = "upsert";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        QSqlQuery created(db);
        created.exec("SELECT created_at, updated_at, name, source FROM share_entries WHERE tth != ''");
        created.next();
        if (created.value(0).toString() != "2000-01-01 00:00:00") {
            if (error)
                *error = "created_at changed";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        if (created.value(1).toString() == "2000-01-01 00:00:00") {
            if (error)
                *error = "updated_at not changed";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        if (created.value(2).toString() != "S01E01_renamed.mkv") {
            if (error)
                *error = "name not updated";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }
        if (created.value(3).toInt() != SourceFileList) {
            if (error)
                *error = "hub upsert demoted file_list source";
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        if (!shareIndexSmokeSearch(idx, db, error)) {
            db.close();
            QSqlDatabase::removeDatabase(conn);
            return false;
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
    return true;
}

#endif
