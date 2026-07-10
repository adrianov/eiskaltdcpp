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
#include "WulforUtil.h"
#include "dcpp/DirectoryListing.h"
#include "dcpp/Util.h"

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
                             QList<QVariantMap> &out)
{
    if (!dir)
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
        if (isStopping())
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
        appendUnique(std::move(row), seen, out);
    }

    for (auto &sub : dir->directories) {
        if (isStopping())
            return;
        walkListing(listing, sub, cid, hubUrl, nick, hubName, ip, seen, out);
    }
}

#endif
