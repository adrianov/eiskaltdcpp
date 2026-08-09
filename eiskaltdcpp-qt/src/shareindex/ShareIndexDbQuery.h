/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
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

/** Torn ART index or already-invalidated handle — reopen cannot heal on-disk pages. */
inline bool isPoisoned(const QString &msg)
{
    return msg.contains(QLatin1String("Failed to delete all rows from index"))
            || msg.contains(QLatin1String("Failed to insert into index"))
            || msg.contains(QLatin1String("database has been invalidated"));
}

inline void noteFailure(duckdb::ExceptionType t, const QString &msg)
{
    if (duckdb::Exception::InvalidatesDatabase(t) || isPoisoned(msg))
        fatalPending() = true;
}

template <typename Fn>
bool guard(QString *err, Fn &&fn)
{
    try {
        return fn();
    } catch (const duckdb::FatalException &e) {
        const QString msg = QString::fromUtf8(e.what());
        noteFailure(duckdb::ExceptionType::FATAL, msg);
        setErr(err, msg);
        return false;
    } catch (const duckdb::InternalException &e) {
        const QString msg = QString::fromUtf8(e.what());
        noteFailure(duckdb::ExceptionType::INTERNAL, msg);
        setErr(err, msg);
        return false;
    } catch (const std::exception &e) {
        const QString msg = QString::fromUtf8(e.what());
        noteFailure(duckdb::ErrorData(e).Type(), msg);
        setErr(err, msg);
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
            const QString msg = res ? QString::fromStdString(res->GetError())
                                    : QStringLiteral("null result");
            if (res && res->HasError())
                noteFailure(res->GetErrorType(), msg);
            setErr(err, msg);
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
            const QString msg = res && res->HasError()
                    ? QString::fromStdString(res->GetError())
                    : QStringLiteral("empty");
            if (res && res->HasError())
                noteFailure(res->GetErrorType(), msg);
            setErr(err, msg);
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
            const QString msg = pending ? QString::fromStdString(pending->GetError())
                                        : QStringLiteral("pending");
            if (pending && pending->HasError())
                noteFailure(pending->GetErrorType(), msg);
            setErr(err, msg);
            return false;
        }
        auto qres = pending->Execute();
        if (!qres || qres->HasError()) {
            const QString msg = qres ? QString::fromStdString(qres->GetError())
                                     : QStringLiteral("execute");
            if (qres && qres->HasError())
                noteFailure(qres->GetErrorType(), msg);
            setErr(err, msg);
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
