/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "ShareIndex.h"

#include <QAtomicInt>
#include <QMutex>
#include <QQueue>

/** Shared write-queue state for ShareIndexQueue / ShareIndexEnqueue. */
namespace ShareIndexWriteQueue {

enum WriteKind { IngestList, IngestCached, UpsertSearch };

struct WriteJob {
    WriteKind kind = IngestList;
    dcpp::UserPtr user;
    QString listPath;
    QString hubUrl;
    QString nick;
    QVariantMap map;
};

extern QMutex writeMutex;
extern QQueue<WriteJob> writeQueue;
extern bool writeWorkerRunning;
extern QAtomicInt writeStopping;
extern const int kHubQueueCap;

bool takeNextJob(WriteJob &job);
void dropOldestHubJob();
void enqueueWrite(WriteJob job);
bool isStopping();

} // namespace ShareIndexWriteQueue

void shareIndexRunWriteWorker();
