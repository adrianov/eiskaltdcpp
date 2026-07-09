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

#include "WulforUtil.h"

#include <QElapsedTimer>

using namespace dcpp;

qint64 ShareIndex::forceIngestListMs(const UserPtr &user, const QString &listPath,
                                     const QString &hubUrl, const QString &nick)
{
    QElapsedTimer timer;
    timer.start();
    waitWritesIdle();
    ingestListSync(user, listPath, hubUrl, nick, true);
    releaseThreadDb();
    return timer.elapsed();
}

void ShareIndex::ingestListSync(const UserPtr &user, const QString &listPath,
                                const QString &hubUrl, const QString &nick,
                                bool force)
{
    if (!user || listPath.isEmpty())
        return;

    open();
    if (!isOpen())
        return;

    const QString cid = _q(user->getCID().toBase32());
    if (cid.isEmpty())
        return;

    // Unchanged list: skip full walk + FTS rebuild (can be minutes for large shares).
    if (!force && !needsListIngest(cid, listPath)) {
        setLastError(QString());
        return;
    }

    QString useNick = nick;
    if (useNick.isEmpty()) {
        WulforUtil *wu = WulforUtil::getInstance();
        if (wu)
            useNick = wu->getNicks(user->getCID(), hubUrl);
    }

    DirectoryListing listing(HintedUser(user, _tq(hubUrl)));
    try {
        listing.loadFile(_tq(listPath));
    } catch (const Exception &e) {
        setLastError(QString("loadFile: %1").arg(_q(e.getError())));
        return;
    } catch (const std::exception &e) {
        setLastError(QString("loadFile std: %1").arg(e.what()));
        return;
    } catch (...) {
        setLastError(QStringLiteral("loadFile unknown"));
        return;
    }

    QList<QVariantMap> rows;
    walkListing(listing, listing.getRoot(), cid, hubUrl, useNick, QString(), rows);
    if (rows.isEmpty()) {
        setLastError(QStringLiteral("walkListing empty"));
        return;
    }

    // Large commits (~10s on M1 SQLite+FTS trigram). Write queue is single-threaded;
    // WAL lets searches continue while this transaction runs.
    const int chunk = 100000;
    for (int offset = 0; offset < rows.size(); ) {
        QSqlDatabase db = threadDb();
        if (!db.isOpen()) {
            setLastError(QStringLiteral("threadDb not open"));
            return;
        }

        db.transaction();

        if (offset == 0) {
            QSqlQuery del(db);
            del.prepare("DELETE FROM share_entries WHERE cid = ? AND source = ?");
            del.addBindValue(cid);
            del.addBindValue(int(SourceFileList));
            if (!del.exec()) {
                setLastError(del.lastError().text());
                db.rollback();
                return;
            }
        }

        QSqlQuery ins(db);
        if (!prepareInsert(ins)) {
            setLastError(ins.lastError().text());
            db.rollback();
            return;
        }

        const int end = qMin(offset + chunk, rows.size());
        for (; offset < end; ++offset) {
            if (!bindInsert(ins, rows.at(offset), SourceFileList) || !ins.exec()) {
                setLastError(ins.lastError().text());
                db.rollback();
                return;
            }
        }

        if (!db.commit()) {
            setLastError(db.lastError().text());
            return;
        }
    }
    rememberListMeta(cid, listPath, rows.size());
    setLastError(QString());
}

#endif
