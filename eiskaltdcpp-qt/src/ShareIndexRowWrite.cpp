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

namespace {

QString sqlText(const QVariant &v)
{
    const QString s = v.toString();
    return s.isNull() ? QStringLiteral("") : s;
}

duckdb::Value sameNull(const QString &value, const QString &canonical, bool useCanon)
{
    if (useCanon && value == canonical)
        return ShareIndexDb::nullVal();
    return ShareIndexDb::strVal(value);
}

} // namespace

/** Normalize one result, then UPDATE-else-INSERT its location.
    File strings equal to share_files become NULL; dirs stay concrete. */
bool ShareIndex::upsertRow(duckdb::Connection &con, const QVariantMap &row, int source)
{
    const bool isDir = row.value("is_dir").toBool();
    const QString tth = sqlText(row.value("tth"));
    const QString cid = sqlText(row.value("cid"));
    const QString path = sqlText(row.value("path"));
    const QString name = sqlText(row.value("name"));
    const QString nameCf = name.toCaseFolded();
    const QString pathCf = path.toCaseFolded();
    const QString ext = row.contains("ext") ? sqlText(row.value("ext")) : fileExt(name, isDir);
    if (cid.isEmpty() || name.isEmpty())
        return false;

    const bool byTth = !isDir && !tth.isEmpty();
    const qint64 size = row.value("size").toLongLong();
    const QString stamp = nowStamp();
    QString err;

    const qint64 userId = ensureUserId(con, row, stamp);
    if (userId < 0)
        return false;

    qint64 fileId = 0;
    QString cName, cPath, cExt, cNameCf, cPathCf;
    if (byTth) {
        fileId = ensureFileId(con, tth, size, name, path, ext,
                              &cName, &cPath, &cExt, &cNameCf, &cPathCf);
        if (fileId < 0)
            return false;
    }

    const duckdb::Value vPath = sameNull(path, cPath, byTth);
    const duckdb::Value vName = sameNull(name, cName, byTth);
    const duckdb::Value vNameCf = sameNull(nameCf, cNameCf, byTth);
    const duckdb::Value vPathCf = sameNull(pathCf, cPathCf, byTth);
    const duckdb::Value vExt = sameNull(ext, cExt, byTth);

    duckdb::vector<duckdb::Value> binds;
    binds.push_back(ShareIndexDb::i64Val(userId));
    binds.push_back(byTth ? ShareIndexDb::i64Val(fileId) : ShareIndexDb::nullVal());
    binds.push_back(vPath);
    binds.push_back(vName);
    binds.push_back(vNameCf);
    binds.push_back(vPathCf);
    binds.push_back(vExt);
    binds.push_back(byTth ? ShareIndexDb::nullVal() : ShareIndexDb::i64Val(size));
    binds.push_back(ShareIndexDb::i64Val(source));
    binds.push_back(ShareIndexDb::i64Val(source));
    binds.push_back(ShareIndexDb::i64Val(userId));
    std::string sql =
        "UPDATE share_locations SET user_id=?, file_id=?, path=?, name=?, name_cf=?, path_cf=?, ext=?,"
        " local_size=?, source=CASE WHEN source=1 OR ?=1 THEN 1 ELSE ? END "
        "WHERE user_id=?";
    if (byTth) {
        sql += " AND file_id IN (SELECT file_id FROM share_files WHERE tth=?)";
        binds.push_back(ShareIndexDb::strVal(tth));
    } else {
        sql += " AND file_id IS NULL AND path=? AND name=? AND is_dir=?";
        binds.push_back(ShareIndexDb::strVal(path));
        binds.push_back(ShareIndexDb::strVal(name));
        binds.push_back(ShareIndexDb::i64Val(isDir ? 1 : 0));
    }

    auto upd = ShareIndexDb::queryMat(con, sql, binds, &err);
    if (!upd) {
        setLastError(err);
        return false;
    }
    if (upd->RowCount() > 0 && ShareIndexDb::qi64(upd->GetValue(0, 0)) > 0) {
        setLastError(QString());
        return true;
    }

    duckdb::vector<duckdb::Value> ins;
    ins.push_back(ShareIndexDb::i64Val(userId));
    ins.push_back(byTth ? ShareIndexDb::i64Val(fileId) : ShareIndexDb::nullVal());
    ins.push_back(vPath);
    ins.push_back(vName);
    ins.push_back(vExt);
    ins.push_back(ShareIndexDb::i64Val(isDir ? 1 : 0));
    ins.push_back(byTth ? ShareIndexDb::nullVal() : ShareIndexDb::i64Val(size));
    ins.push_back(ShareIndexDb::i64Val(source));
    ins.push_back(ShareIndexDb::strVal(stamp));
    ins.push_back(vNameCf);
    ins.push_back(vPathCf);
    auto res = ShareIndexDb::queryMat(con,
        "INSERT INTO share_locations ("
        "user_id, file_id, path, name, ext, is_dir, local_size, source, created_at,"
        " name_cf, path_cf) VALUES (?,?,?,?,?,?,?,?,?,?,?)", ins, &err);
    if (!res) {
        setLastError(err);
        return false;
    }
    setLastError(QString());
    return true;
}

#endif
