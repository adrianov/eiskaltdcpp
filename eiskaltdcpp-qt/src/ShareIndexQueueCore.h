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

/** Serialized ShareIndex jobs, including online-user queue matching. */
namespace ShareIndexWriteQueue {

enum WriteKind { OpenDb, MatchQueue, IngestList, UpsertSearch, BumpShowHits, RemoveTth };

struct WriteJob {
    WriteKind kind = IngestList;
    dcpp::UserPtr user;
    QString listPath;
    QString hubUrl;
    QString nick;
    QString cid;
    QString tth;
    QVariantMap map;
    QList<qint64> ids;
    dcpp::UserList users;
};

extern QMutex writeMutex;
extern QQueue<WriteJob> writeQueue;
extern bool writeWorkerRunning;
extern QAtomicInt writeStopping;
extern const int kHubQueueCap;

bool isStopping();
bool takeNextJob(WriteJob &job);
void dropOldestHubJob();
void enqueueWrite(WriteJob job);
QList<QVariantMap> takeHubUpserts();
dcpp::UserList takeMatchUsers();

} // namespace ShareIndexWriteQueue

void shareIndexRunWriteWorker();
