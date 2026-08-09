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

#include <QDateTime>

using namespace dcpp;

ShareIndex::ShareIndex()
#ifndef USE_QT_SQLITE
    : opened(0)
#endif
{
#ifdef USE_QT_SQLITE
    searchEpoch.storeRelease(0);
    ShareIndexWriteQueue::writeStopping.storeRelease(0);
#endif
}

ShareIndex::~ShareIndex()
{
#ifdef USE_QT_SQLITE
    stopWrites();
    closeDb();
#else
    opened.storeRelease(0);
#endif
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

#ifdef USE_QT_SQLITE

void ShareIndex::setLastError(const QString &err)
{
    QMutexLocker lock(&errorMutex);
    lastSqlError = err;
}

#else

void ShareIndex::open() {}

void ShareIndex::openAsync() { open(); }

void ShareIndex::upsertFromSearch(const QVariantMap &) {}

void ShareIndex::ingestList(const UserPtr &, const QString &, const QString &, const QString &) {}

void ShareIndex::matchQueue(const UserList &) {}

void ShareIndex::removeTth(const QString &, const QString &) {}

void ShareIndex::removeUser(const QString &) {}

qint64 ShareIndex::forceIngestListMs(const UserPtr &, const QString &, const QString &, const QString &)
{
    return 0;
}

void ShareIndex::waitWritesIdle() {}

void ShareIndex::stopWrites() {}

QList<QVariantMap> ShareIndex::search(const SearchFilter &) { return {}; }

QHash<QString, QList<ShareIndex::IndexUser>>
ShareIndex::usersByTth(const QStringList &, qint64, int)
{
    return {};
}

QHash<QString, ShareIndex::MediaInfo>
ShareIndex::mediaByTth(const QStringList &)
{
    return {};
}

void ShareIndex::upsertMedia(const QHash<QString, MediaInfo> &) {}

ShareIndex::IndexStats ShareIndex::indexStats() { return {}; }

bool ShareIndex::needsListIngest(const QString &, const QString &) { return false; }

void ShareIndex::releaseThreadDb() {}

bool ShareIndex::smokeCheck(QString *) { return true; }

#endif
