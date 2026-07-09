/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#include <QDateTime>

#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;

ShareIndex::ShareIndex() : opened(0)
{
}

ShareIndex::~ShareIndex()
{
#ifdef USE_QT_SQLITE
    stopWrites();
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

    // 30 GiB WAL + full disk → sqlite3_wal_checkpoint aborts; drop sidecars first.
    prepareDbFile();

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return;

    // Prefer incremental auto_vacuum + 16 KiB pages on empty DBs only.
    // Existing indexes keep their layout (no wipe / no full VACUUM).
    if (!ensureAutoVacuum(db)) {
        if (!recreateForVacuum())
            return;
        db = threadDb();
        if (!db.isOpen() || !ensureAutoVacuum(db))
            return;
    }

    if (!ensureSchema(db) || !ensureCap(db) || !ensureFts(db))
        return;

    opened.storeRelease(1);
#endif
}

void ShareIndex::openAsync()
{
#ifdef USE_QT_SQLITE
    if (opened.loadAcquire())
        return;
    if (dbFile.isEmpty())
        dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.sqlite";

    using namespace ShareIndexWriteQueue;
    WriteJob job;
    job.kind = OpenDb;
    enqueueWrite(job);
#else
    open();
#endif
}

#ifdef USE_QT_SQLITE

void ShareIndex::setLastError(const QString &err)
{
    QMutexLocker lock(&errorMutex);
    lastSqlError = err;
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

void ShareIndex::stopWrites() {}

QList<QVariantMap> ShareIndex::search(const SearchFilter &) { return {}; }

ShareIndex::IndexStats ShareIndex::indexStats() { return {}; }

bool ShareIndex::needsListIngest(const QString &, const QString &) { return false; }

void ShareIndex::releaseThreadDb() {}

bool ShareIndex::smokeCheck(QString *) { return true; }

#endif
