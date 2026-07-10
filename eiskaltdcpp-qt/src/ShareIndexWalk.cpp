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
#include "dcpp/DirectoryListing.h"
#include "dcpp/Util.h"
#include "WulforUtil.h"

#include <QDir>
#include <utility>

using namespace dcpp;
using namespace ShareIndexWriteQueue;

namespace {

void appendUnique(QVariantMap row, QSet<QString> &seen, QList<QVariantMap> &out)
{
    const QString key = ShareIndexDb::rowUkey(
        row.value("cid").toString(), row.value("tth").toString(),
        row.value("path").toString(), row.value("name").toString(),
        row.value("is_dir").toBool());
    if (!seen.contains(key)) {
        seen.insert(key);
        out.append(std::move(row));
    }
}

} // namespace

void ShareIndex::walkListing(DirectoryListing &listing, DirectoryListing::Directory *dir,
                             const QString &cid, const QString &hubUrl,
                             const QString &nick, const QString &hubName,
                             const QString &ip, QSet<QString> &seen,
                             QList<QVariantMap> &out,
                             bool backfill)
{
    if (!dir)
        return;
    if (backfill && shouldAbortBackfill())
        return;

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
        appendUnique(std::move(row), seen, out);
    }

    for (auto &f : dir->files) {
        if (isStopping() || (backfill && shouldAbortBackfill()))
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
        row["shared_ts"] = qulonglong(f->getTS());
        appendUnique(std::move(row), seen, out);
    }

    for (auto &sub : dir->directories) {
        if (isStopping() || (backfill && shouldAbortBackfill()))
            return;
        walkListing(listing, sub, cid, hubUrl, nick, hubName, ip, seen, out, backfill);
    }
}

/** CLI: ingest every cached list on this thread (no write worker). */
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
        if (isStopping())
            return;

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
        ingestListSync(user, fullPath, QString(), nick, false, false);
    }
}

#endif
