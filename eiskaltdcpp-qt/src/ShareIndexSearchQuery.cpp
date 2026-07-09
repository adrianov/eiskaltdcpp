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

namespace {

static const char *kSelectCols =
    "SELECT e.name, e.size, e.tth, e.path, e.nick, e.free_slots, e.all_slots, "
    "e.ip, e.hub_name, e.hub_url, e.cid, e.is_dir, e.ext FROM share_entries e ";

/** Quote a term for FTS5 MATCH; trigram needs ≥3 chars for substring hits. */
QString matchToken(const QString &term)
{
    QString s = term.trimmed().toCaseFolded();
    if (s.isEmpty() || s.startsWith('-') || s.size() < 3)
        return QString();
    s.replace('"', "\"\"");
    return "\"" + s + "\"";
}

} // namespace

QString ShareIndex::filterSql(const SearchFilter &filter, QVariantList &binds) const
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
            binds << e.toUpper();
        }
        parts << QString("e.ext IN (%1)").arg(ph.join(','));
    }

    if (filter.size > 0 && filter.sizeMode != SearchManager::SIZE_DONTCARE) {
        if (filter.sizeMode == SearchManager::SIZE_ATLEAST) {
            parts << "e.size >= ?";
            binds << filter.size;
        } else if (filter.sizeMode == SearchManager::SIZE_ATMOST) {
            parts << "e.size <= ?";
            binds << filter.size;
        }
    }

    if (parts.isEmpty())
        return QString();
    return " AND " + parts.join(" AND ");
}

QList<QVariantMap> ShareIndex::rowsFromQuery(QSqlQuery &q)
{
    QList<QVariantMap> out;
    while (q.next()) {
        QVariantMap map;
        map["FILE"] = q.value(0);
        map["SIZE"] = q.value(1).toULongLong();
        map["TTH"] = q.value(2);
        map["PATH"] = q.value(3);
        map["NICK"] = q.value(4);
        map["FSLS"] = q.value(5).isNull() ? 0 : q.value(5).toULongLong();
        map["ASLS"] = q.value(6).isNull() ? 0 : q.value(6).toULongLong();
        map["IP"] = q.value(7).toString();
        map["HUB"] = q.value(8);
        map["HOST"] = q.value(9);
        map["CID"] = q.value(10);
        map["ISDIR"] = q.value(11).toInt() != 0;
        map["EXT"] = q.value(12).toString();
        out.append(map);
    }
    return out;
}

QList<QVariantMap> ShareIndex::searchFts(QSqlDatabase &db, const SearchFilter &filter)
{
    QVariantList binds;

    if (filter.isHash && !filter.terms.isEmpty()) {
        QSqlQuery q(db);
        q.prepare(QString(kSelectCols) + "WHERE e.tth = ?" + filterSql(filter, binds) + " LIMIT ?");
        q.addBindValue(filter.terms.first());
        for (const auto &b : binds)
            q.addBindValue(b);
        q.addBindValue(filter.limit);
        if (!q.exec()) {
            setLastError(q.lastError().text());
            return {};
        }
        setLastError(QString());
        return rowsFromQuery(q);
    }

    QStringList tokens;
    for (const QString &t : filter.terms) {
        const QString tok = matchToken(t);
        if (!tok.isEmpty())
            tokens << tok;
    }
    if (tokens.isEmpty())
        return {};

    // AND of quoted phrases: each term is a substring; order does not matter.
    const QString match = tokens.join(" AND ");

    QSqlQuery q(db);
    QString sql = QString(kSelectCols) +
                  "JOIN share_entries_fts ON share_entries_fts.rowid = e.id "
                  "WHERE share_entries_fts MATCH ?" +
                  filterSql(filter, binds) + " LIMIT ?";
    q.prepare(sql);
    q.addBindValue(match);
    for (const auto &b : binds)
        q.addBindValue(b);
    q.addBindValue(filter.limit);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return {};
    }
    setLastError(QString());
    return rowsFromQuery(q);
}

#endif
