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
#include <QFileInfo>
#include <QSqlQuery>
#include <QStorageInfo>
#include <QThread>

namespace {

/** Drop -wal/-shm when checkpoint is impossible (WAL larger than free disk). */
void recoverOversizedWal(const QString &dbPath)
{
    const QString walPath = dbPath + QStringLiteral("-wal");
    const QFileInfo wal(walPath);
    if (!wal.exists())
        return;

    const qint64 walBytes = wal.size();
    if (walBytes < (qint64(1) << 30)) // < 1 GiB
        return;

    const QStorageInfo storage(QFileInfo(dbPath).absolutePath());
    if (!storage.isValid())
        return;
    // Need headroom to rewrite pages from WAL into the main DB.
    if (storage.bytesAvailable() > walBytes + (qint64(1) << 30))
        return;

    QFile::remove(walPath);
    QFile::remove(dbPath + QStringLiteral("-shm"));
}

} // namespace

void ShareIndex::prepareDbFile()
{
    if (dbFile.isEmpty())
        return;
    recoverOversizedWal(dbFile);
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
    // Avoid auto-checkpoint during schema open (huge -wal files hang/abort the UI).
    pragma.exec("PRAGMA wal_autocheckpoint=0");
    pragma.exec("PRAGMA journal_size_limit=268435456"); // 256 MiB
    pragma.exec("PRAGMA busy_timeout=5000");
    pragma.exec("PRAGMA synchronous=NORMAL");
    // Cap trigger DELETE must fire FTS/count AFTER DELETE triggers.
    pragma.exec("PRAGMA recursive_triggers=ON");
    // ~64 MiB page cache (negative = KiB); helps multi-GB ShareIndex reads.
    pragma.exec("PRAGMA cache_size=-65536");
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

#endif
