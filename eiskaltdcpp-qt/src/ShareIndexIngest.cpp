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

#include "dcpp/ClientManager.h"
#include "dcpp/CID.h"
#include "WulforUtil.h"

using namespace dcpp;

void ShareIndex::walkListing(DirectoryListing &listing, DirectoryListing::Directory *dir,
                             const QString &cid, const QString &hubUrl,
                             const QString &nick, const QString &hubName,
                             QList<QVariantMap> &out)
{
    if (!dir)
        return;

    // Skip listing root; index named directories and all files.
    if (dir != listing.getRoot() && !dir->getName().empty()) {
        QVariantMap row;
        row["cid"] = cid;
        row["hub_url"] = hubUrl;
        row["tth"] = QString();
        row["path"] = _q(listing.getPath(dir->getParent()));
        row["name"] = _q(dir->getName());
        row["ext"] = QString();
        row["is_dir"] = true;
        row["size"] = qint64(dir->getSize());
        row["nick"] = nick;
        row["hub_name"] = hubName;
        out.append(row);
    }

    for (auto &f : dir->files) {
        if (!f || f->getName().empty())
            continue;

        QVariantMap row;
        row["cid"] = cid;
        row["hub_url"] = hubUrl;
        row["tth"] = _q(f->getTTH().toBase32());
        row["path"] = _q(listing.getPath(f));
        row["name"] = _q(f->getName());
        row["ext"] = fileExt(row["name"].toString(), false);
        row["is_dir"] = false;
        row["size"] = qint64(f->getSize());
        row["nick"] = nick;
        row["hub_name"] = hubName;
        row["bitrate"] = int(f->mediaInfo.bitrate);
        row["resolution"] = _q(f->mediaInfo.resolution);
        row["video_info"] = _q(f->mediaInfo.video_info);
        row["audio_info"] = _q(f->mediaInfo.audio_info);
        row["hit"] = qulonglong(f->getHit());
        row["shared_ts"] = qulonglong(f->getTS());
        out.append(row);
    }

    for (auto &sub : dir->directories)
        walkListing(listing, sub, cid, hubUrl, nick, hubName, out);
}

void ShareIndex::ingestList(const UserPtr &user, const QString &listPath,
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
    } catch (const std::exception &) {
        return;
    } catch (...) {
        return;
    }

    QList<QVariantMap> rows;
    walkListing(listing, listing.getRoot(), cid, hubUrl, useNick, QString(), rows);

    QMutexLocker lock(&mutex);
    QSqlDatabase db = QSqlDatabase::database("ShareIndex");
    if (!db.isOpen())
        return;

    db.transaction();

    QSqlQuery del(db);
    del.prepare("DELETE FROM share_entries WHERE cid = ? AND source = ?");
    del.addBindValue(cid);
    del.addBindValue(int(SourceFileList));
    if (!del.exec()) {
        db.rollback();
        return;
    }

    for (const auto &row : rows) {
        if (!upsertRow(db, row, SourceFileList)) {
            db.rollback();
            return;
        }
    }

    db.commit();
}

#endif
