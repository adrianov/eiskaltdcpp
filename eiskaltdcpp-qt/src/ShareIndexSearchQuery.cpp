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

static const char *kSelectCols =
    "SELECT coalesce(e.name,f.name), coalesce(f.size,e.local_size,0), "
    "coalesce(f.tth,''), coalesce(e.path,f.path), "
    "u.nick, u.free_slots, u.all_slots, u.ip, u.hub_name, u.hub_url, "
    "u.cid, e.is_dir, coalesce(e.ext,f.ext) FROM share_locations e "
    "JOIN share_users u ON u.user_id=e.user_id "
    "LEFT JOIN share_files f ON f.file_id=e.file_id ";

/** Case-folded term for contains(); keep ≥3 chars like FTS5 trigram. */
QString matchToken(const QString &term)
{
    QString s = term.trimmed().toCaseFolded();
    if (s.isEmpty() || s.startsWith('-') || s.size() < 3)
        return QString();
    return s;
}

void bindTokens(duckdb::vector<duckdb::Value> &binds, const QStringList &tokens)
{
    for (const QString &tok : tokens) {
        binds.push_back(ShareIndexDb::strVal(tok));
        binds.push_back(ShareIndexDb::strVal(tok));
    }
}

QString fileTokenSql(int n)
{
    QString sql;
    for (int i = 0; i < n; ++i)
        sql += " AND (contains(name_cf, ?) OR contains(path_cf, ?))";
    return sql;
}

QString locTokenSql(int n)
{
    QString sql;
    for (int i = 0; i < n; ++i) {
        sql += " AND (contains(coalesce(e.name_cf,f.name_cf), ?) "
               "OR contains(coalesce(e.path_cf,f.path_cf), ?))";
    }
    return sql;
}

bool isInterrupt(const QString &err)
{
    return err.contains(QLatin1String("Interrupt"), Qt::CaseInsensitive);
}

} // namespace

QList<QVariantMap> ShareIndex::searchFts(duckdb::Connection &con, const SearchFilter &filter)
{
    QString err;

    if (filter.isHash && !filter.terms.isEmpty()) {
        duckdb::vector<duckdb::Value> binds;
        QString sql = QString(kSelectCols) + "WHERE f.tth = ?";
        binds.push_back(ShareIndexDb::strVal(filter.terms.first()));
        sql += filterSql(filter, binds);
        sql += " LIMIT ?";
        binds.push_back(ShareIndexDb::i64Val(filter.limit));
        auto mat = ShareIndexDb::queryMat(con, sql.toStdString(), binds, &err);
        if (!mat) {
            setLastError(isInterrupt(err) ? QString() : err);
            return {};
        }
        setLastError(QString());
        return rowsFromResult(*mat);
    }

    QStringList tokens;
    for (const QString &t : filter.terms) {
        const QString tok = matchToken(t);
        if (!tok.isEmpty())
            tokens << tok;
    }
    if (tokens.isEmpty())
        return {};

    QList<QVariantMap> out;

    // 1) Unique share_files first (canonical name/path). Skip locations with
    //    overrides so display text still matches the query (override branch below).
    if (!filter.dirsOnly) {
        duckdb::vector<duckdb::Value> binds;
        QString sql = QString(kSelectCols);
        sql += "WHERE e.name_cf IS NULL AND e.path_cf IS NULL "
               "AND e.file_id IN (SELECT file_id FROM share_files WHERE 1=1";
        sql += fileTokenSql(tokens.size());
        bindTokens(binds, tokens);
        sql += " LIMIT ?)";
        binds.push_back(ShareIndexDb::i64Val(filter.limit));
        sql += filterSql(filter, binds);
        sql += " LIMIT ?";
        binds.push_back(ShareIndexDb::i64Val(filter.limit));

        auto mat = ShareIndexDb::queryMat(con, sql.toStdString(), binds, &err);
        if (!mat) {
            setLastError(isInterrupt(err) ? QString() : err);
            return {};
        }
        out = rowsFromResult(*mat);
        if (out.size() >= filter.limit) {
            setLastError(QString());
            return out;
        }
    }

    // 2) Directories and/or per-location name/path overrides (smaller gated set).
    const int remain = filter.limit - out.size();
    if (remain <= 0)
        return out;

    duckdb::vector<duckdb::Value> binds;
    QString sql = QString(kSelectCols) + "WHERE ";
    sql += filter.filesOnly
            ? "e.is_dir = 0 AND (e.name_cf IS NOT NULL OR e.path_cf IS NOT NULL)"
            : filter.dirsOnly
              ? "e.is_dir = 1"
              : "(e.is_dir = 1 OR e.name_cf IS NOT NULL OR e.path_cf IS NOT NULL)";
    sql += locTokenSql(tokens.size());
    bindTokens(binds, tokens);
    sql += filterSql(filter, binds);
    sql += " LIMIT ?";
    binds.push_back(ShareIndexDb::i64Val(remain));

    auto mat = ShareIndexDb::queryMat(con, sql.toStdString(), binds, &err);
    if (!mat) {
        setLastError(isInterrupt(err) ? QString() : err);
        return out; // keep any file hits already found
    }
    out += rowsFromResult(*mat);
    setLastError(QString());
    return out;
}

#endif
