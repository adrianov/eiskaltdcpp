/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndexQueueCore.h"
#include "ShareBrowser.h"

#ifdef USE_QT_SQLITE

#include <QThread>

using namespace ShareIndexWriteQueue;

namespace {

void startWriteWorker()
{
    AsyncRunner *runner = new AsyncRunner();
    runner->setRunFunction([]() { shareIndexRunWriteWorker(); });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

} // namespace

namespace ShareIndexWriteQueue {

void enqueueWrite(WriteJob job)
{
    QMutexLocker lock(&writeMutex);
    if (isStopping())
        return;

    if (job.kind == OpenDb) {
        for (const WriteJob &p : writeQueue) {
            if (p.kind == OpenDb)
                return;
        }
        writeQueue.prepend(job);
    } else if (job.kind == IngestList) {
        for (const WriteJob &p : writeQueue) {
            if (p.kind == IngestList && p.listPath == job.listPath)
                return;
        }
        writeQueue.enqueue(job);
    } else if (job.kind == UpsertSearch) {
        int hubJobs = 0;
        for (const WriteJob &p : writeQueue) {
            if (p.kind == UpsertSearch)
                ++hubJobs;
        }
        if (hubJobs >= kHubQueueCap)
            dropOldestHubJob();
        writeQueue.enqueue(job);
    } else {
        writeQueue.enqueue(job);
    }

    if (!writeWorkerRunning) {
        writeWorkerRunning = true;
        lock.unlock();
        startWriteWorker();
    }
}

} // namespace ShareIndexWriteQueue

void ShareIndex::ingestList(const UserPtr &user, const QString &listPath,
                            const QString &hubUrl, const QString &nick)
{
    if (!user || listPath.isEmpty())
        return;

    WriteJob job;
    job.kind = IngestList;
    job.user = user;
    job.listPath = listPath;
    job.hubUrl = hubUrl;
    job.nick = nick;
    enqueueWrite(job);
}

void ShareIndex::matchQueue(const UserList &users)
{
    if (users.empty())
        return;
    WriteJob job;
    job.kind = MatchQueue;
    job.users = users;
    enqueueWrite(job);
}

void ShareIndex::removeTth(const QString &cid, const QString &tth)
{
    if (cid.isEmpty() || tth.isEmpty())
        return;
    WriteJob job;
    job.kind = RemoveTth;
    job.cid = cid;
    job.tth = tth;
    enqueueWrite(job);
}

void ShareIndex::removeUser(const QString &cid)
{
    if (cid.isEmpty())
        return;
    {
        QMutexLocker lock(&writeMutex);
        for (const WriteJob &j : writeQueue) {
            if (j.kind == RemoveUser && j.cid == cid)
                return;
        }
    }
    WriteJob job;
    job.kind = RemoveUser;
    job.cid = cid;
    enqueueWrite(job);
}

void ShareIndex::upsertFromSearch(const QVariantMap &map)
{
    WriteJob job;
    job.kind = UpsertSearch;
    job.map = map;
    enqueueWrite(job);
}

void ShareIndex::waitWritesIdle()
{
    for (;;) {
        {
            QMutexLocker lock(&writeMutex);
            if (!writeWorkerRunning && writeQueue.isEmpty())
                return;
        }
        QThread::msleep(50);
    }
}

void ShareIndex::stopWrites()
{
    writeStopping.storeRelease(1);
    {
        QMutexLocker lock(&writeMutex);
        writeQueue.clear();
    }
    for (;;) {
        {
            QMutexLocker lock(&writeMutex);
            if (!writeWorkerRunning)
                return;
        }
        QThread::msleep(50);
    }
}

#else

void ShareIndex::stopWrites() {}

#endif
