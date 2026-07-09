/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareBrowser.h"

#ifdef USE_QT_SQLITE

#include <QMutex>
#include <QQueue>
#include <QThread>

using namespace dcpp;

namespace {

enum WriteKind { IngestList, IngestCached, UpsertSearch };

struct WriteJob {
    WriteKind kind = IngestList;
    UserPtr user;
    QString listPath;
    QString hubUrl;
    QString nick;
    QVariantMap map;
};

QMutex writeMutex;
QQueue<WriteJob> writeQueue;
bool writeWorkerRunning = false;
const int kHubQueueCap = 2000;

bool takeNextJob(WriteJob &job)
{
    for (int i = 0; i < writeQueue.size(); ++i) {
        if (writeQueue.at(i).kind != UpsertSearch) {
            job = writeQueue.takeAt(i);
            return true;
        }
    }
    if (writeQueue.isEmpty())
        return false;
    job = writeQueue.dequeue();
    return true;
}

void dropOldestHubJob()
{
    for (int i = 0; i < writeQueue.size(); ++i) {
        if (writeQueue.at(i).kind == UpsertSearch) {
            writeQueue.removeAt(i);
            return;
        }
    }
}

void startWriteWorker();

} // namespace

void shareIndexRunWriteWorker()
{
    ShareIndex::getInstance()->drainWriteQueue();
}

void ShareIndex::drainWriteQueue()
{
    for (;;) {
        WriteJob job;
        {
            QMutexLocker lock(&writeMutex);
            if (!takeNextJob(job)) {
                // Enqueue holds the same mutex, so a new job cannot appear here.
                writeWorkerRunning = false;
                disconnectThreadDb();
                return;
            }
        }

        switch (job.kind) {
        case IngestList:
            ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
            break;
        case IngestCached:
            ingestCachedListsSync();
            break;
        case UpsertSearch:
            upsertFromSearchSync(job.map);
            break;
        }
    }
}

namespace {

void startWriteWorker()
{
    AsyncRunner *runner = new AsyncRunner();
    runner->setRunFunction([]() { shareIndexRunWriteWorker(); });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

void enqueueWrite(WriteJob job)
{
    QMutexLocker lock(&writeMutex);
    if (job.kind == IngestList) {
        for (const WriteJob &pending : writeQueue) {
            if (pending.kind == IngestList && pending.listPath == job.listPath)
                return;
        }
    } else if (job.kind == UpsertSearch) {
        int hubJobs = 0;
        for (const WriteJob &pending : writeQueue) {
            if (pending.kind == UpsertSearch)
                ++hubJobs;
        }
        if (hubJobs >= kHubQueueCap)
            dropOldestHubJob();
    }
    writeQueue.enqueue(job);
    if (!writeWorkerRunning) {
        writeWorkerRunning = true;
        lock.unlock();
        startWriteWorker();
    }
}

} // namespace

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

void ShareIndex::ingestCachedLists()
{
    WriteJob job;
    job.kind = IngestCached;
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

#endif
