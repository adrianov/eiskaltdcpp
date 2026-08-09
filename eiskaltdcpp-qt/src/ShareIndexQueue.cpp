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

#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

using namespace ShareIndexWriteQueue;

void shareIndexRunWriteWorker()
{
    ShareIndex *idx = ShareIndex::getInstance();
    if (!idx)
        return;
    idx->drainWriteQueue();
}

void ShareIndex::drainWriteQueue()
{
    for (;;) {
        if (isStopping()) {
            QMutexLocker lock(&writeMutex);
            writeQueue.clear();
            writeWorkerRunning = false;
            disconnectThreadDb();
            return;
        }

        WriteJob job;
        {
            QMutexLocker lock(&writeMutex);
            if (!takeNextJob(job)) {
                writeWorkerRunning = false;
                disconnectThreadDb();
                return;
            }
        }

        try {
            switch (job.kind) {
            case OpenDb:
                open();
                break;
            case MatchQueue: {
                dcpp::UserList users = job.users;
                {
                    QMutexLocker lock(&writeMutex);
                    const dcpp::UserList pending = takeMatchUsers();
                    users.insert(users.end(), pending.begin(), pending.end());
                }
                matchQueueSync(users);
                break;
            }
            case RemoveTth:
                removeTthSync(job.cid, job.tth);
                break;
            case RemoveUser:
                removeUserSync(job.cid);
                break;
            case IngestList:
                ingestListSync(job.user, job.listPath, job.hubUrl, job.nick);
                break;
            case UpsertMedia:
                upsertMediaSync(job.media);
                break;
            case UpsertSearch: {
                QList<QVariantMap> maps;
                maps.append(job.map);
                {
                    QMutexLocker lock(&writeMutex);
                    maps.append(takeHubUpserts());
                }
                upsertFromSearchBatchSync(maps);
                break;
            }
            }
        } catch (const duckdb::FatalException &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ExceptionType::FATAL, msg);
            setLastError(msg);
        } catch (const duckdb::InternalException &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ExceptionType::INTERNAL, msg);
            setLastError(msg);
        } catch (const std::exception &e) {
            const QString msg = QString::fromUtf8(e.what());
            ShareIndexDb::noteFailure(duckdb::ErrorData(e).Type(), msg);
            setLastError(msg);
        } catch (...) {
            setLastError(QStringLiteral("share index write failed"));
        }

        // Open already erases+retries once on a poisoned DB; avoid a second rebuild loop.
        if (ShareIndexDb::takeFatal() && !(job.kind == OpenDb && isOpen()))
            recoverDb();
    }
}

#endif
