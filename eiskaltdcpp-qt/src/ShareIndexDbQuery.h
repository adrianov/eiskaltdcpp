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

/** DuckDB exec/query helpers + fatal-error tracking for ShareIndex. */
namespace ShareIndexDb {

inline void setErr(QString *err, const QString &msg)
{
    if (err)
        *err = msg;
}

/** Set when DuckDB reports a DB-invalidating error; consumed by the write worker. */
inline bool &fatalPending()
{
    static thread_local bool f = false;
    return f;
}

inline bool takeFatal()
{
    const bool f = fatalPending();
    fatalPending() = false;
    return f;
}

inline void noteType(duckdb::ExceptionType t)
{
    if (duckdb::Exception::InvalidatesDatabase(t))
        fatalPending() = true;
}

template <typename Fn>
bool guard(QString *err, Fn &&fn)
{
    try {
        return fn();
    } catch (const duckdb::FatalException &e) {
        noteType(duckdb::ExceptionType::FATAL);
        setErr(err, QString::fromUtf8(e.what()));
        return false;
    } catch (const duckdb::InternalException &e) {
        noteType(duckdb::ExceptionType::INTERNAL);
        setErr(err, QString::fromUtf8(e.what()));
        return false;
    } catch (const std::exception &e) {
        noteType(duckdb::ErrorData(e).Type());
        setErr(err, QString::fromUtf8(e.what()));
        return false;
    } catch (...) {
        setErr(err, QStringLiteral("duckdb error"));
        return false;
    }
}

inline bool execOk(duckdb::Connection &con, const std::string &sql, QString *err = nullptr)
{
    return guard(err, [&]() {
        auto res = con.Query(sql);
        if (!res || res->HasError()) {
            if (res && res->HasError())
                noteType(res->GetErrorType());
            setErr(err, res ? QString::fromStdString(res->GetError())
                            : QStringLiteral("null result"));
            return false;
        }
        return true;
    });
}

inline bool scalarI64(duckdb::Connection &con, const std::string &sql, qint64 *out,
                      QString *err = nullptr)
{
    return guard(err, [&]() {
        auto res = con.Query(sql);
        if (!res || res->HasError() || res->RowCount() == 0) {
            if (res && res->HasError())
                noteType(res->GetErrorType());
            setErr(err, res && res->HasError() ? QString::fromStdString(res->GetError())
                                               : QStringLiteral("empty"));
            return false;
        }
        if (out)
            *out = res->GetValue(0, 0).IsNull() ? 0 : res->GetValue(0, 0).GetValue<int64_t>();
        return true;
    });
}

/** Materialized query with bound parameters (C++14-safe). */
inline duckdb::unique_ptr<duckdb::MaterializedQueryResult>
queryMat(duckdb::Connection &con, const std::string &sql, duckdb::vector<duckdb::Value> &binds,
         QString *err = nullptr)
{
    duckdb::unique_ptr<duckdb::MaterializedQueryResult> out;
    guard(err, [&]() {
        auto pending = con.PendingQuery(sql, binds, duckdb::QueryResultOutputType::FORCE_MATERIALIZED);
        if (!pending || pending->HasError()) {
            if (pending && pending->HasError())
                noteType(pending->GetErrorType());
            setErr(err, pending ? QString::fromStdString(pending->GetError())
                                : QStringLiteral("pending"));
            return false;
        }
        auto qres = pending->Execute();
        if (!qres || qres->HasError()) {
            if (qres && qres->HasError())
                noteType(qres->GetErrorType());
            setErr(err, qres ? QString::fromStdString(qres->GetError())
                             : QStringLiteral("execute"));
            return false;
        }
        auto *mat = dynamic_cast<duckdb::MaterializedQueryResult *>(qres.get());
        if (!mat) {
            setErr(err, QStringLiteral("not materialized"));
            return false;
        }
        qres.release();
        out = duckdb::unique_ptr<duckdb::MaterializedQueryResult>(mat);
        return true;
    });
    return out;
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

} // namespace ShareIndexDb

#endif
