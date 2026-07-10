/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#ifdef USE_QT_SQLITE

#include <duckdb.hpp>

#include <QString>
#include <QVariant>

/** DuckDB Value helpers for ShareIndex (Qt ↔ DuckDB). */
namespace ShareIndexDb {

inline duckdb::Value nullVal()
{
    return duckdb::Value();
}

inline duckdb::Value strVal(const QString &s)
{
    return duckdb::Value(s.toStdString());
}

inline duckdb::Value strVal(const QVariant &v)
{
    // Prefer '' over SQL NULL so NOT NULL text columns (hub_url, …) never fail.
    if (!v.isValid() || v.isNull())
        return strVal(QStringLiteral(""));
    const QString s = v.toString();
    return s.isNull() ? strVal(QStringLiteral("")) : strVal(s);
}

inline duckdb::Value i64Val(qint64 v)
{
    return duckdb::Value::BIGINT(v);
}

inline duckdb::Value i64Val(const QVariant &v)
{
    if (!v.isValid() || v.isNull())
        return nullVal();
    return i64Val(v.toLongLong());
}

inline duckdb::Value i32Val(const QVariant &v)
{
    if (!v.isValid() || v.isNull())
        return nullVal();
    return duckdb::Value::INTEGER(v.toInt());
}

inline QString qstr(const duckdb::Value &v)
{
    if (v.IsNull())
        return QString();
    return QString::fromStdString(v.GetValue<std::string>());
}

inline qint64 qi64(const duckdb::Value &v)
{
    if (v.IsNull())
        return 0;
    return v.GetValue<int64_t>();
}

inline bool execOk(duckdb::Connection &con, const std::string &sql, QString *err = nullptr)
{
    auto res = con.Query(sql);
    if (res->HasError()) {
        if (err)
            *err = QString::fromStdString(res->GetError());
        return false;
    }
    return true;
}

/** Materialized query with bound parameters (C++14-safe). */
inline duckdb::unique_ptr<duckdb::MaterializedQueryResult>
queryMat(duckdb::Connection &con, const std::string &sql, duckdb::vector<duckdb::Value> &binds,
         QString *err = nullptr)
{
    auto pending = con.PendingQuery(sql, binds, duckdb::QueryResultOutputType::FORCE_MATERIALIZED);
    if (!pending || pending->HasError()) {
        if (err)
            *err = pending ? QString::fromStdString(pending->GetError()) : QStringLiteral("pending");
        return nullptr;
    }
    auto qres = pending->Execute();
    if (!qres || qres->HasError()) {
        if (err)
            *err = qres ? QString::fromStdString(qres->GetError()) : QStringLiteral("execute");
        return nullptr;
    }
    auto *mat = dynamic_cast<duckdb::MaterializedQueryResult *>(qres.get());
    if (!mat) {
        if (err)
            *err = QStringLiteral("not materialized");
        return nullptr;
    }
    qres.release();
    return duckdb::unique_ptr<duckdb::MaterializedQueryResult>(mat);
}

inline duckdb::unique_ptr<duckdb::MaterializedQueryResult>
query1(duckdb::Connection &con, const std::string &sql, const duckdb::Value &a, QString *err = nullptr)
{
    duckdb::vector<duckdb::Value> binds;
    binds.push_back(a);
    return queryMat(con, sql, binds, err);
}

inline duckdb::unique_ptr<duckdb::MaterializedQueryResult>
query2(duckdb::Connection &con, const std::string &sql, const duckdb::Value &a, const duckdb::Value &b,
       QString *err = nullptr)
{
    duckdb::vector<duckdb::Value> binds;
    binds.push_back(a);
    binds.push_back(b);
    return queryMat(con, sql, binds, err);
}

/** Upsert key: file with TTH → f:cid:tth; else → p:cid:path:name:is_dir. */
inline QString rowUkey(const QString &cid, const QString &tth, const QString &path,
                       const QString &name, bool isDir)
{
    if (!isDir && !tth.isEmpty())
        return QStringLiteral("f:") + cid + QLatin1Char(':') + tth;
    return QStringLiteral("p:") + cid + QLatin1Char(':') + path + QLatin1Char(':') + name
            + QLatin1Char(':') + (isDir ? QLatin1Char('1') : QLatin1Char('0'));
}

} // namespace ShareIndexDb

#endif
