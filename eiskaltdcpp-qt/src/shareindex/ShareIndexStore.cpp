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

#include "shareindex/ShareIndexStore.h"

#ifdef USE_QT_SQLITE

#include <QDir>
#include <QFile>
#include <QThread>

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

#endif
