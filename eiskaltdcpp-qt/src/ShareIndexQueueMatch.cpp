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
#include "dcpp/Encoder.h"
#include "dcpp/ListCache.h"
#include "dcpp/QueueManager.h"

using namespace dcpp;

namespace {

const size_t CID_BATCH = 250;
const size_t TTH_BATCH = 500;
const size_t ROOT_LEN = 39;

std::string bindMarks(size_t count)
{
    std::string sql;
    for (size_t i = 0; i < count; ++i)
        sql += i ? ",?" : "?";
    return sql;
}

} // namespace

/** Query only current queue roots, then let QueueManager verify and add each source. */
void ShareIndex::matchQueueSync(const UserList &users)
{
    open();
    if (!isOpen())
        return;

    QueueManager *qm = QueueManager::getInstance();
    const vector<TTHValue> roots = qm->getQueuedTTHs();
    if (roots.empty())
        return;

    unordered_map<string, UserPtr> online;
    for (const UserPtr &user : users) {
        if (user && ListCache::matchesShare(user))
            online.emplace(user->getCID().toBase32(), user);
    }
    if (online.empty())
        return;

    vector<string> cids;
    vector<string> tths;
    for (const auto &user : online)
        cids.push_back(user.first);
    for (const TTHValue &root : roots)
        tths.push_back(root.toBase32());

    unordered_map<string, vector<QueueManager::SourceMatch>> matches;
    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    for (size_t ci = 0; ci < cids.size(); ci += CID_BATCH) {
        const size_t cn = std::min(CID_BATCH, cids.size() - ci);
        for (size_t ti = 0; ti < tths.size(); ti += TTH_BATCH) {
            const size_t tn = std::min(TTH_BATCH, tths.size() - ti);
            const std::string sql =
                "SELECT cid,tth,size FROM share_entries WHERE source=1 AND is_dir=0"
                " AND cid IN (" + bindMarks(cn) + ")"
                " AND tth IN (" + bindMarks(tn) + ")";

            duckdb::vector<duckdb::Value> binds;
            for (size_t i = 0; i < cn; ++i)
                binds.push_back(ShareIndexDb::strVal(QString::fromStdString(cids[ci + i])));
            for (size_t i = 0; i < tn; ++i)
                binds.push_back(ShareIndexDb::strVal(QString::fromStdString(tths[ti + i])));

            QString error;
            auto result = ShareIndexDb::queryMat(*con, sql, binds, &error);
            if (!result) {
                setLastError(error);
                return;
            }
            for (idx_t row = 0; row < result->RowCount(); ++row) {
                const string cid = ShareIndexDb::qstr(result->GetValue(0, row)).toStdString();
                const string tth = ShareIndexDb::qstr(result->GetValue(1, row)).toStdString();
                if (tth.size() == ROOT_LEN && Encoder::isBase32(tth))
                    matches[cid].emplace_back(TTHValue(tth), ShareIndexDb::qi64(result->GetValue(2, row)));
            }
        }
    }

    for (const auto &entry : matches) {
        const auto user = online.find(entry.first);
        if (user != online.end() && user->second->isOnline())
            qm->matchSources(HintedUser(user->second, Util::emptyString), entry.second);
    }
}

#endif
