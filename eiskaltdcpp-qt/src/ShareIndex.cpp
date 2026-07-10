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
#ifdef USE_QT_SQLITE
    ShareIndexWriteQueue::writeStopping.storeRelease(0);
#endif
}

ShareIndex::~ShareIndex()
{
#ifdef USE_QT_SQLITE
    stopWrites();
    {
        QMutexLocker lock(&connMutex);
        threadConns.clear();
    }
    duck.reset();
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
        dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.duckdb";

    try {
        duck = std::make_unique<duckdb::DuckDB>(dbFile.toStdString());
    } catch (const std::exception &e) {
        setLastError(QString::fromUtf8(e.what()));
        duck.reset();
        return;
    }

    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    // Bound RAM so the index does not dominate the process.
    ShareIndexDb::execOk(*con, "SET memory_limit='1GB'");
    ShareIndexDb::execOk(*con, "SET threads=2");

    if (!ensureSchema(*con) || !ensureCap(*con) || !ensureFts(*con))
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
        dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.duckdb";

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

void ShareIndex::matchQueue(const UserList &) {}

void ShareIndex::removeTth(const QString &, const QString &) {}

qint64 ShareIndex::forceIngestListMs(const UserPtr &, const QString &, const QString &, const QString &)
{
    return 0;
}

void ShareIndex::waitWritesIdle() {}

void ShareIndex::stopWrites() {}

QList<QVariantMap> ShareIndex::search(const SearchFilter &) { return {}; }

ShareIndex::IndexStats ShareIndex::indexStats() { return {}; }

bool ShareIndex::needsListIngest(const QString &, const QString &) { return false; }

void ShareIndex::releaseThreadDb() {}

bool ShareIndex::smokeCheck(QString *) { return true; }

#endif
