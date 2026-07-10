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

#include <QThread>

duckdb::Connection *ShareIndex::threadConn()
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

void ShareIndex::disconnectThreadDb()
{
    const quintptr tid = quintptr(QThread::currentThreadId());
    QMutexLocker lock(&connMutex);
    threadConns.remove(tid);
}

void ShareIndex::releaseThreadDb()
{
    disconnectThreadDb();
}

#endif
