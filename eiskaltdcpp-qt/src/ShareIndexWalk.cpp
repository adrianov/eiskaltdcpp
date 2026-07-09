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

#include "ShareIndexQueueCore.h"
#include "dcpp/ClientManager.h"
#include "dcpp/DirectoryListing.h"
#include "WulforUtil.h"

#include <QDir>

using namespace dcpp;

void ShareIndex::walkListing(DirectoryListing &listing, DirectoryListing::Directory *dir,
                             const QString &cid, const QString &hubUrl,
                             const QString &nick, const QString &hubName,
                             const QString &ip, QList<QVariantMap> &out)
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
        row["ip"] = ip;
        out.append(row);
    }

    for (auto &f : dir->files) {
        if (ShareIndexWriteQueue::isStopping())
            return;
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
        row["ip"] = ip;
        row["bitrate"] = int(f->mediaInfo.bitrate);
        row["resolution"] = _q(f->mediaInfo.resolution);
        row["video_info"] = _q(f->mediaInfo.video_info);
        row["audio_info"] = _q(f->mediaInfo.audio_info);
        row["hit"] = qulonglong(f->getHit());
        row["shared_ts"] = qulonglong(f->getTS());
        out.append(row);
    }

    for (auto &sub : dir->directories) {
        if (ShareIndexWriteQueue::isStopping())
            return;
        walkListing(listing, sub, cid, hubUrl, nick, hubName, ip, out);
    }
}

void ShareIndex::ingestCachedListsSync()
{
    open();
    if (!isOpen())
        return;

    const QString listDir = _q(Util::getListPath());
    QDir dir(listDir);
    if (!dir.exists())
        return;

    const QStringList names = dir.entryList(QStringList() << "*.xml.bz2" << "*.xml", QDir::Files);
    for (const QString &fn : names) {
        if (ShareIndexWriteQueue::isStopping())
            return;
        // Let an interactive list ingest run before the rest of backfill.
        if (pendingListIngest()) {
            requeueCachedIngest();
            return;
        }

        const QString fullPath = dir.absoluteFilePath(fn);
        const UserPtr user = DirectoryListing::getUserFromFilename(_tq(fullPath));
        if (!user)
            continue;

        QString base = fn;
        if (base.endsWith(QLatin1String(".xml.bz2"), Qt::CaseInsensitive))
            base.chop(8);
        else if (base.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive))
            base.chop(4);

        QString nick;
        const int dot = base.lastIndexOf(QLatin1Char('.'));
        if (dot > 0)
            nick = base.left(dot);

        setLastError(QString());
        ingestListSync(user, fullPath, QString(), nick);
    }
}

#endif
