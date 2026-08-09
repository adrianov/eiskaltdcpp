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

#include "FinishedTransfers.h"

#ifdef USE_QT_SQLITE
#include <QtSql>

namespace {

QString tableSql(QSqlDatabase &db, const QString &name)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT sql FROM sqlite_master WHERE type='table' AND name=?;"));
    q.bindValue(0, name);
    if (!q.exec() || !q.next())
        return QString();
    return q.value(0).toString();
}

void migrateFilesTable(QSqlDatabase &db)
{
    const QString sql = tableSql(db, QStringLiteral("files"));
    if (sql.isEmpty())
        return;

    if (!sql.contains(QStringLiteral("TARGET TEXT PRIMARY KEY"), Qt::CaseInsensitive)) {
        QSqlQuery q(db);
        q.exec(QStringLiteral("BEGIN"));
        q.exec(QStringLiteral(
            "CREATE TABLE files_new ("
            "TARGET TEXT PRIMARY KEY, FNAME TEXT, TIME TEXT, PATH TEXT, USERS TEXT, "
            "TR TEXT, SPEED TEXT, CRC32 INTEGER, ELAP TEXT, FULL INTEGER, TTH TEXT);"));
        q.exec(QStringLiteral(
            "INSERT OR REPLACE INTO files_new "
            "(TARGET, FNAME, TIME, PATH, USERS, TR, SPEED, CRC32, ELAP, FULL) "
            "SELECT TARGET, FNAME, TIME, PATH, USERS, TR, SPEED, CRC32, ELAP, FULL "
            "FROM files WHERE TARGET IS NOT NULL AND TARGET != '';"));
        q.exec(QStringLiteral("DROP TABLE files;"));
        q.exec(QStringLiteral("ALTER TABLE files_new RENAME TO files;"));
        q.exec(QStringLiteral("COMMIT"));
        return;
    }

    if (!sql.contains(QStringLiteral("TTH"), Qt::CaseInsensitive)) {
        QSqlQuery q(db);
        q.exec(QStringLiteral("ALTER TABLE files ADD COLUMN TTH TEXT;"));
    }
}

void migrateUsersTable(QSqlDatabase &db)
{
    const QString sql = tableSql(db, QStringLiteral("users"));
    if (sql.isEmpty() || sql.contains(QStringLiteral("CID TEXT PRIMARY KEY"), Qt::CaseInsensitive))
        return;

    QSqlQuery q(db);
    q.exec(QStringLiteral("BEGIN"));
    q.exec(QStringLiteral(
        "CREATE TABLE users_new ("
        "CID TEXT PRIMARY KEY, NICK TEXT, TIME TEXT, FILES TEXT, "
        "TR TEXT, SPEED TEXT, ELAP TEXT, FULL INTEGER);"));
    q.exec(QStringLiteral(
        "INSERT OR REPLACE INTO users_new "
        "(CID, NICK, TIME, FILES, TR, SPEED, ELAP, FULL) "
        "SELECT CID, NICK, TIME, FILES, TR, SPEED, ELAP, FULL "
        "FROM users WHERE CID IS NOT NULL AND CID != '';"));
    q.exec(QStringLiteral("DROP TABLE users;"));
    q.exec(QStringLiteral("ALTER TABLE users_new RENAME TO users;"));
    q.exec(QStringLiteral("COMMIT"));
}

} // namespace
#endif

template <bool isUpload>
void FinishedTransfers<isUpload>::openDatabase()
{
#ifdef USE_QT_SQLITE
    db = QSqlDatabase::addDatabase("QSQLITE", (isUpload? "FinishedUploads" : "FinishedDownloads"));
    db_file = _q(Util::getPath(Util::PATH_USER_CONFIG)) + (isUpload? "FinishedUploads.sqlite" : "FinishedDownloads.sqlite");

    db.setDatabaseName(db_file);
    db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    db_opened = db.open();

    if (!db_opened)
        return;

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS files ("
           "TARGET TEXT PRIMARY KEY, "
           "FNAME TEXT, TIME TEXT, PATH TEXT, USERS TEXT, TR TEXT, SPEED TEXT, "
           "CRC32 INTEGER, ELAP TEXT, FULL INTEGER, TTH TEXT);");

    q.exec("CREATE TABLE IF NOT EXISTS users ("
           "CID TEXT PRIMARY KEY, "
           "NICK TEXT, TIME TEXT, FILES TEXT, TR TEXT, SPEED TEXT, ELAP TEXT, FULL INTEGER);");

    migrateFilesTable(db);
    migrateUsersTable(db);

    // Rebuild search "already have" index here (sync), not in the async UI loader.
    if (!isUpload) {
        QSqlQuery idx(db);
        if (idx.exec(QStringLiteral(
                "SELECT TARGET, TTH FROM files "
                "WHERE FULL != 0 AND TTH IS NOT NULL AND length(TTH) = 39;"))) {
            while (idx.next()) {
                const string target = _tq(idx.value(0).toString());
                if (isFileListPath(target))
                    continue;
                FinishedManager::getInstance()->setDownloadTarget(
                            _tq(idx.value(1).toString()), target);
            }
        }
    }
#endif
}

template void FinishedTransfers<true>::openDatabase();
template void FinishedTransfers<false>::openDatabase();
