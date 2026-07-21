/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#include "dcpp/CID.h"
#include "dcpp/PeerConnectHub.h"

#ifdef USE_QT_SQLITE

using namespace dcpp;

namespace {

QVariantMap searchMapToRow(const QVariantMap &map)
{
    const bool isDir = map.value("ISDIR").toBool();
    const QString name = map.value("FILE").toString();

    QVariantMap row;
    row["cid"] = map.value("CID");
    row["hub_url"] = map.value("HOST");
    row["tth"] = map.value("TTH");
    row["path"] = map.value("PATH");
    row["name"] = name;
    row["ext"] = ShareIndex::fileExt(name, isDir);
    row["is_dir"] = isDir;
    row["size"] = map.value("SIZE");
    row["nick"] = map.value("NICK");
    row["hub_name"] = map.value("HUB");
    row["free_slots"] = map.value("FSLS");
    row["all_slots"] = map.value("ASLS");
    row["ip"] = map.value("IP");
    return row;
}

} // namespace

void ShareIndex::upsertFromSearchBatchSync(const QList<QVariantMap> &maps)
{
    if (maps.isEmpty())
        return;

    open();
    if (!isOpen())
        return;

    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    if (!ShareIndexDb::execOk(*con, "BEGIN TRANSACTION"))
        return;
    for (const QVariantMap &map : maps) {
        if (ShareIndexWriteQueue::isStopping()) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return;
        }
        const QString cid = map.value(QStringLiteral("CID")).toString();
        if (cid.size() == 39 && PeerConnectHub::isUnreachableCid(CID(cid.toStdString())))
            continue;
        if (!upsertRow(*con, searchMapToRow(map), SourceHubSearch)) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            return;
        }
    }
    if (!ShareIndexDb::execOk(*con, "COMMIT")) {
        setLastError(QStringLiteral("commit failed"));
        return;
    }
    if (!removeOrphans(*con))
        return;
    refreshEntryCount(*con);
    // No CHECKPOINT here: hub SRs are frequent; vacuum after list ingest only.
}

#endif
