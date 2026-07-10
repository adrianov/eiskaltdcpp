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

/**
 * Single-writer job queue for ShareIndex.
 *
 * One file list = one IngestList job (no monolithic backfill loop).
 * Priority: OpenDb → interactive list → hub SR → show-hits → backfill list.
 * Enqueueing an interactive list sets abortBackfill so a huge load/walk yields.
 */
namespace ShareIndexWriteQueue {

enum WriteKind { OpenDb, IngestList, UpsertSearch, BumpShowHits, ScanCached };

struct WriteJob {
    WriteKind kind = IngestList;
    dcpp::UserPtr user;
    QString listPath;
    QString hubUrl;
    QString nick;
    QVariantMap map;
    QList<qint64> ids;
    bool backfill = false;
};

extern QMutex writeMutex;
extern QQueue<WriteJob> writeQueue;
extern bool writeWorkerRunning;
extern QAtomicInt writeStopping;
/** Set when an interactive IngestList is queued; backfill load/walk/write checks this. */
extern QAtomicInt abortBackfill;
extern const int kHubQueueCap;

bool isStopping();
bool shouldAbortBackfill();
void requestAbortBackfill();
void clearAbortBackfill();

bool takeNextJob(WriteJob &job);
void dropOldestHubJob();
void enqueueWrite(WriteJob job);
QList<QVariantMap> takeHubUpserts();

} // namespace ShareIndexWriteQueue

void shareIndexRunWriteWorker();
