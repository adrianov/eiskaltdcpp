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
#include <QThread>

#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;

ShareIndex::ShareIndex() : opened(0)
{
}

ShareIndex::~ShareIndex()
{
#ifdef USE_QT_SQLITE
    waitWritesIdle();
    disconnectThreadDb();
#endif
    opened.storeRelease(0);
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

QString ShareIndex::lastError() const
{
    QMutexLocker lock(&errorMutex);
    return lastSqlError;
}

void ShareIndex::open()
{
#ifdef USE_QT_SQLITE
    if (opened.loadAcquire())
        return;

    QMutexLocker lock(&openMutex);
    if (opened.loadAcquire())
        return;

    if (dbFile.isEmpty())
        dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.sqlite";

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return;

    if (!ensureSchema(db) || !ensureFts(db))
        return;

    opened.storeRelease(1);
#endif
}

#ifdef USE_QT_SQLITE

void ShareIndex::setLastError(const QString &err)
{
    QMutexLocker lock(&errorMutex);
    lastSqlError = err;
}

QSqlDatabase ShareIndex::threadDb()
{
    if (dbFile.isEmpty())
        return QSqlDatabase();

    const QString name = QStringLiteral("ShareIndex_%1")
            .arg(quintptr(QThread::currentThreadId()));

    QMutexLocker lock(&connMutex);
    if (QSqlDatabase::contains(name)) {
        QSqlDatabase db = QSqlDatabase::database(name);
        if (!db.isOpen())
            db.open();
        return db;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(dbFile);
    if (!db.open())
        return QSqlDatabase();
    // WAL: concurrent readers while one writer runs (write queue is single-threaded).
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA journal_mode=WAL");
    pragma.exec("PRAGMA busy_timeout=30000");
    pragma.exec("PRAGMA synchronous=NORMAL");
    return db;
}

void ShareIndex::disconnectThreadDb()
{
    const QString name = QStringLiteral("ShareIndex_%1")
            .arg(quintptr(QThread::currentThreadId()));
    QMutexLocker lock(&connMutex);
    if (!QSqlDatabase::contains(name))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(name);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(name);
}

void ShareIndex::releaseThreadDb()
{
    disconnectThreadDb();
}

#else

void ShareIndex::upsertFromSearch(const QVariantMap &) {}

void ShareIndex::ingestList(const UserPtr &, const QString &, const QString &, const QString &) {}

qint64 ShareIndex::forceIngestListMs(const UserPtr &, const QString &, const QString &, const QString &)
{
    return 0;
}

void ShareIndex::ingestCachedLists() {}

void ShareIndex::waitWritesIdle() {}

QList<QVariantMap> ShareIndex::search(const SearchFilter &) { return {}; }

ShareIndex::IndexStats ShareIndex::indexStats() { return {}; }

bool ShareIndex::needsListIngest(const QString &, const QString &) { return false; }

void ShareIndex::releaseThreadDb() {}

bool ShareIndex::smokeCheck(QString *) { return true; }

#endif
