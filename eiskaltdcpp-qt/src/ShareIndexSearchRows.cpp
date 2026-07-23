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

#include "dcpp/SearchManager.h"

using namespace dcpp;

QString ShareIndex::filterSql(const SearchFilter &filter, duckdb::vector<duckdb::Value> &binds) const
{
    QStringList parts;

    if (filter.dirsOnly) {
        parts << "e.is_dir = 1";
    } else if (filter.filesOnly) {
        parts << "e.is_dir = 0";
    }

    if (!filter.extensions.isEmpty()) {
        QStringList ph;
        for (const QString &e : filter.extensions) {
            ph << "?";
            binds.push_back(ShareIndexDb::strVal(e.toUpper()));
        }
        parts << QString("coalesce(e.ext,f.ext) IN (%1)").arg(ph.join(','));
    }

    if (filter.size > 0 && filter.sizeMode != SearchManager::SIZE_DONTCARE) {
        if (filter.sizeMode == SearchManager::SIZE_ATLEAST) {
            parts << "coalesce(f.size,e.local_size,0) >= ?";
            binds.push_back(ShareIndexDb::i64Val(filter.size));
        } else if (filter.sizeMode == SearchManager::SIZE_ATMOST) {
            parts << "coalesce(f.size,e.local_size,0) <= ?";
            binds.push_back(ShareIndexDb::i64Val(filter.size));
        }
    }

    if (parts.isEmpty())
        return QString();
    return " AND " + parts.join(" AND ");
}

QList<QVariantMap> ShareIndex::rowsFromResult(duckdb::MaterializedQueryResult &res)
{
    QList<QVariantMap> out;
    const idx_t n = res.RowCount();
    out.reserve(int(n));
    for (idx_t r = 0; r < n; ++r) {
        QVariantMap map;
        map["FILE"] = ShareIndexDb::qstr(res.GetValue(0, r));
        map["SIZE"] = quint64(ShareIndexDb::qi64(res.GetValue(1, r)));
        map["TTH"] = ShareIndexDb::qstr(res.GetValue(2, r));
        map["PATH"] = ShareIndexDb::qstr(res.GetValue(3, r));
        map["NICK"] = ShareIndexDb::qstr(res.GetValue(4, r));
        map["FSLS"] = res.GetValue(5, r).IsNull() ? 0 : quint64(ShareIndexDb::qi64(res.GetValue(5, r)));
        map["ASLS"] = res.GetValue(6, r).IsNull() ? 0 : quint64(ShareIndexDb::qi64(res.GetValue(6, r)));
        map["IP"] = ShareIndexDb::qstr(res.GetValue(7, r));
        map["HUB"] = ShareIndexDb::qstr(res.GetValue(8, r));
        map["HOST"] = ShareIndexDb::qstr(res.GetValue(9, r));
        map["CID"] = ShareIndexDb::qstr(res.GetValue(10, r));
        map["ISDIR"] = ShareIndexDb::qi64(res.GetValue(11, r)) != 0;
        map["EXT"] = ShareIndexDb::qstr(res.GetValue(12, r));
        out.append(map);
    }
    return out;
}

#endif
