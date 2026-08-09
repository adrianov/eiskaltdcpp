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

#include "search/SearchLocalPath.h"
#include "WulforUtil.h"

#include "dcpp/File.h"
#include "dcpp/FinishedManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/Util.h"

#include <QDesktopServices>
#include <QUrl>

#ifdef USE_QT_SQLITE
#include <QtSql>
#endif

using namespace dcpp;

namespace SearchLocalPath {

static QString fromShare(const QString &tth)
{
    if (tth.isEmpty())
        return QString();

    ShareManager *sm = ShareManager::getInstance();
    try {
        const string path = sm->toReal(sm->toVirtual(TTHValue(_tq(tth))));
        if (File::getSize(path) > -1)
            return _q(path);
    } catch (...) {}

    return QString();
}

#ifdef USE_QT_SQLITE
/** FinishedDownloads.sqlite has no TTH column; match unique FNAME + on-disk size. */
static QString fromFinishedDb(const QString &fileName, qint64 size)
{
    if (fileName.isEmpty() || size <= 0)
        return QString();

    static const QString conn = QStringLiteral("SearchFinishedRo");
    if (!QSqlDatabase::contains(conn)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(_q(Util::getPath(Util::PATH_USER_CONFIG))
                           + QStringLiteral("FinishedDownloads.sqlite"));
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=1000"));
        if (!db.open())
            return QString();
    }

    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(QStringLiteral("SELECT TARGET FROM files WHERE FNAME = ? AND FULL = 1;"));
    q.addBindValue(fileName);
    if (!q.exec())
        return QString();

    QString found;
    while (q.next()) {
        const QString target = q.value(0).toString();
        if (File::getSize(_tq(target)) != size)
            continue;
        if (!found.isEmpty())
            return QString();
        found = target;
    }
    return found;
}
#endif

static QString fromFinished(const QString &tth, qint64 size, const QString &fileName)
{
    // size <= 0 is not authoritative (missing ESIZE / dirs); avoid matching a 0-byte file.
    if (tth.isEmpty() || size <= 0)
        return QString();

    const string path = FinishedManager::getInstance()->getTarget(_tq(tth), size, _tq(fileName));
    if (!path.empty() && File::getSize(path) == size)
        return _q(path);

#ifdef USE_QT_SQLITE
    const QString dbPath = fromFinishedDb(fileName, size);
    if (!dbPath.isEmpty()) {
        FinishedManager::getInstance()->setDownloadTarget(_tq(tth), _tq(dbPath));
        return dbPath;
    }
#endif
    return QString();
}

QString resolve(const QString &tth, qint64 size, const QString &fileName)
{
    const QString shared = fromShare(tth);
    if (!shared.isEmpty())
        return shared;
    return fromFinished(tth, size, fileName);
}

bool isQueued(const QString &tth)
{
    if (tth.isEmpty())
        return false;
    // Same TTH index DONT_DL_ALREADY_QUEUED uses; empty-TTH filelists never reach here.
    return !QueueManager::getInstance()->getTargets(TTHValue(_tq(tth))).empty();
}

bool openFile(const QString &path)
{
    if (path.isEmpty())
        return false;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool openDirectory(const QString &path)
{
    if (path.isEmpty())
        return false;
    return WulforUtil::revealPath(path);
}

} // namespace SearchLocalPath
