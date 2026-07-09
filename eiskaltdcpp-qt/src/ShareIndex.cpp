/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#include <QDateTime>

#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;

ShareIndex::ShareIndex() : opened(false)
#ifdef USE_QT_SQLITE
    , ftsReady(false)
#endif
{
}

ShareIndex::~ShareIndex()
{
#ifdef USE_QT_SQLITE
    QMutexLocker lock(&mutex);
    if (opened && QSqlDatabase::contains("ShareIndex")) {
        {
            QSqlDatabase db = QSqlDatabase::database("ShareIndex");
            if (db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase("ShareIndex");
    }
#endif
    opened = false;
}

QString ShareIndex::nowStamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

QString ShareIndex::fileExt(const QString &name, bool isDir)
{
    if (isDir || name.isEmpty())
        return QString();

    const int dot = name.lastIndexOf('.');
    if (dot < 0 || dot == name.size() - 1)
        return QString();

    // Match SearchModel: QFileInfo::suffix().toUpper()
    return name.mid(dot + 1).toUpper();
}

void ShareIndex::open()
{
#ifdef USE_QT_SQLITE
    QMutexLocker lock(&mutex);
    if (opened)
        return;

    if (dbFile.isEmpty())
        dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.sqlite";

    QSqlDatabase db = connectDb("ShareIndex");
    if (!db.isOpen())
        return;

    if (!ensureSchema(db))
        return;

    ftsReady = ensureFts(db);
    opened = true;
#endif
}

#ifdef USE_QT_SQLITE

QSqlDatabase ShareIndex::connectDb(const QString &connName)
{
    if (QSqlDatabase::contains(connName))
        return QSqlDatabase::database(connName);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbFile);
    db.open();
    return db;
}

void ShareIndex::disconnectDb(const QString &connName)
{
    if (!QSqlDatabase::contains(connName))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

bool ShareIndex::ensureSchema(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS share_entries ("
            "id INTEGER PRIMARY KEY,"
            "cid TEXT NOT NULL,"
            "hub_url TEXT NOT NULL DEFAULT '',"
            "tth TEXT NOT NULL DEFAULT '',"
            "path TEXT NOT NULL,"
            "name TEXT NOT NULL,"
            "ext TEXT NOT NULL DEFAULT '',"
            "is_dir INTEGER NOT NULL DEFAULT 0,"
            "size INTEGER NOT NULL DEFAULT 0,"
            "nick TEXT NOT NULL DEFAULT '',"
            "hub_name TEXT NOT NULL DEFAULT '',"
            "free_slots INTEGER,"
            "all_slots INTEGER,"
            "ip TEXT,"
            "bitrate INTEGER,"
            "resolution TEXT,"
            "video_info TEXT,"
            "audio_info TEXT,"
            "hit INTEGER,"
            "shared_ts INTEGER,"
            "source INTEGER NOT NULL,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL"
            ")"))
        return false;

    // Migrate DBs created before ext existed.
    q.exec("ALTER TABLE share_entries ADD COLUMN ext TEXT NOT NULL DEFAULT ''");

    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS share_entries_file_tth "
           "ON share_entries(cid, tth) WHERE is_dir = 0 AND tth != ''");
    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS share_entries_path_name "
           "ON share_entries(cid, path, name, is_dir) WHERE is_dir = 1 OR tth = ''");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_cid ON share_entries(cid)");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_updated ON share_entries(updated_at)");
    q.exec("CREATE INDEX IF NOT EXISTS share_entries_ext ON share_entries(ext)");
    return true;
}

#else

void ShareIndex::upsertFromSearch(const QVariantMap &) {}

void ShareIndex::ingestList(const UserPtr &, const QString &, const QString &, const QString &) {}

QList<QVariantMap> ShareIndex::search(const SearchFilter &) { return {}; }

bool ShareIndex::smokeCheck(QString *) { return true; }

#endif
