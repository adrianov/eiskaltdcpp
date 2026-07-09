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

using namespace dcpp;

void ShareIndex::ingestListSync(const UserPtr &user, const QString &listPath,
                                const QString &hubUrl, const QString &nick)
{
    if (!user || listPath.isEmpty())
        return;

    open();
    if (!opened)
        return;

    const QString cid = _q(user->getCID().toBase32());
    if (cid.isEmpty())
        return;

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
        lastSqlError = QString("loadFile: %1").arg(_q(e.getError()));
        return;
    } catch (const std::exception &e) {
        lastSqlError = QString("loadFile std: %1").arg(e.what());
        return;
    } catch (...) {
        lastSqlError = QStringLiteral("loadFile unknown");
        return;
    }

    QList<QVariantMap> rows;
    walkListing(listing, listing.getRoot(), cid, hubUrl, useNick, QString(), rows);
    if (rows.isEmpty()) {
        lastSqlError = QStringLiteral("walkListing empty");
        return;
    }

    // Large commits (~10s on M1 SQLite+FTS trigram); release mutex between chunks.
    const int chunk = 100000;
    for (int offset = 0; offset < rows.size(); ) {
        QMutexLocker lock(&mutex);
        QSqlDatabase db = threadDb();
        if (!db.isOpen()) {
            lastSqlError = QStringLiteral("threadDb not open");
            return;
        }

        db.transaction();

        if (offset == 0) {
            QSqlQuery del(db);
            del.prepare("DELETE FROM share_entries WHERE cid = ? AND source = ?");
            del.addBindValue(cid);
            del.addBindValue(int(SourceFileList));
            if (!del.exec()) {
                lastSqlError = del.lastError().text();
                db.rollback();
                return;
            }
        }

        QSqlQuery ins(db);
        if (!prepareInsert(ins)) {
            lastSqlError = ins.lastError().text();
            db.rollback();
            return;
        }

        const int end = qMin(offset + chunk, rows.size());
        for (; offset < end; ++offset) {
            if (!bindInsert(ins, rows.at(offset), SourceFileList) || !ins.exec()) {
                lastSqlError = ins.lastError().text();
                db.rollback();
                return;
            }
        }

        if (!db.commit()) {
            lastSqlError = db.lastError().text();
            return;
        }
    }
    lastSqlError.clear();
}

#endif
