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

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

#include <QDir>
#include <QFile>
#include <QThread>

#include "dcpp/Util.h"
#include "WulforUtil.h"

using namespace dcpp;

duckdb::Connection *ShareIndexStore::threadConn()
{
    if (!duck)
        return nullptr;

    const quintptr tid = quintptr(QThread::currentThreadId());
    QMutexLocker lock(&connMutex);
    auto it = threadConns.find(tid);
    if (it != threadConns.end())
        return it.value().get();

    auto con = std::make_shared<duckdb::Connection>(*duck);
    threadConns.insert(tid, con);
    return con.get();
}

void ShareIndexStore::disconnectThread()
{
    const quintptr tid = quintptr(QThread::currentThreadId());
    QMutexLocker lock(&connMutex);
    threadConns.remove(tid);
}

void ShareIndexStore::releaseAll()
{
    QMutexLocker lock(&connMutex);
    threadConns.clear();
    duck.reset();
    opened.storeRelease(0);
}

bool ShareIndexStore::openDuck(QString *err)
{
    try {
        duck = std::make_unique<duckdb::DuckDB>(dbFile.toStdString());
        return true;
    } catch (const std::exception &e) {
        if (err)
            *err = QString::fromUtf8(e.what());
        duck.reset();
        return false;
    }
}

void ShareIndexStore::wipeFiles()
{
    if (dbFile.isEmpty())
        return;
    QFile::remove(dbFile);
    QFile::remove(dbFile + QStringLiteral(".wal"));
    QDir(dbFile + QStringLiteral(".tmp")).removeRecursively();
}

duckdb::Connection *ShareIndex::threadConn()
{
    return store.threadConn();
}

void ShareIndex::disconnectThreadDb()
{
    store.disconnectThread();
}

void ShareIndex::releaseThreadDb()
{
    disconnectThreadDb();
}

void ShareIndex::closeDb()
{
    cancelSearch();
    {
        QMutexLocker lock(&searchMu);
        activeSearchCon = nullptr;
    }
    store.releaseAll();
}

void ShareIndex::wipeDbFiles()
{
    store.wipeFiles();
}

bool ShareIndex::finishOpen()
{
    duckdb::Connection *con = threadConn();
    if (!con)
        return false;

    // Cap RAM so the share index cannot dominate the process.
    ShareIndexDb::execOk(*con, "SET memory_limit='1GB'");
    ShareIndexDb::execOk(*con, "SET threads=2");

    if (!compactLegacyDb())
        return false;

    con = threadConn();
    if (!con || !ensureSchema(*con) || !ensureCap(*con))
        return false;

    // Mark open before soft prune so Search HUD is not blank during cap work.
    store.setOpen(true);
    pruneExcess(*con);
    if (ShareIndexDb::takeFatal()) {
        store.setOpen(false);
        return false;
    }
    return true;
}

void ShareIndex::open()
{
    if (store.isOpen())
        return;

    QMutexLocker lock(&store.openMutex);
    if (store.isOpen())
        return;

    if (store.dbFile.isEmpty())
        store.dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.duckdb";

    const QString oldFile = store.dbFile + QStringLiteral(".migrate-old");
    if (!QFile::exists(store.dbFile) && QFile::exists(oldFile))
        QFile::rename(oldFile, store.dbFile);
    else if (QFile::exists(store.dbFile))
        QFile::remove(oldFile);

    auto tryOpen = [this]() -> bool {
        QString err;
        if (!store.openDuck(&err)) {
            setLastError(err);
            return false;
        }
        return finishOpen();
    };

    if (tryOpen()) {
        setLastError(QString());
        return;
    }

    // Poisoned handle or torn ART: erase once and open empty (lists re-ingest).
    const QString err = lastError();
    closeDb();
    if (!ShareIndexDb::isPoisoned(err))
        return;
    wipeDbFiles();
    if (tryOpen())
        setLastError(QString());
}

void ShareIndex::openAsync()
{
    if (store.isOpen())
        return;
    if (store.dbFile.isEmpty())
        store.dbFile = _q(Util::getPath(Util::PATH_USER_CONFIG)) + "ShareIndex.duckdb";

    using namespace ShareIndexWriteQueue;
    WriteJob job;
    job.kind = OpenDb;
    enqueueWrite(job);
}

void ShareIndex::recoverDb()
{
    const QString err = lastError();
    closeDb();
    // Reopen alone cannot heal torn ART pages — erase and rebuild empty.
    if (ShareIndexDb::isPoisoned(err))
        wipeDbFiles();
    open();
    if (isOpen())
        setLastError(QString());
}

#endif
