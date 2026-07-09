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

#include <QSqlQuery>

namespace {

const qint64 kMaxEntries = 5000000;

bool upsertMeta(QSqlDatabase &db, const QString &key, qint64 value, QString *err)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO share_index_meta(key, value) VALUES(?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value);
    if (!q.exec()) {
        if (err)
            *err = q.lastError().text();
        return false;
    }
    return true;
}

qint64 metaValue(QSqlDatabase &db, const QString &key, qint64 fallback = -1)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT value FROM share_index_meta WHERE key = ?"));
    q.addBindValue(key);
    if (!q.exec() || !q.next())
        return fallback;
    return q.value(0).toLongLong();
}

bool hasTrigger(QSqlDatabase &db, const char *name)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='trigger' AND name=? LIMIT 1"));
    q.addBindValue(QLatin1String(name));
    return q.exec() && q.next();
}

bool metaTableExists(QSqlDatabase &db)
{
    QSqlQuery q(db);
    return q.exec(QStringLiteral(
               "SELECT 1 FROM sqlite_master WHERE type='table' "
               "AND name='share_index_meta' LIMIT 1"))
            && q.next();
}

} // namespace

bool ShareIndex::ensureCap(QSqlDatabase &db)
{
    // Startup must stay read-only when possible: any DDL/DML against a multi-GB
    // WAL can call sqlite3_wal_checkpoint and hang/abort (disk full).

    if (!metaTableExists(db)) {
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral(
                "CREATE TABLE share_index_meta ("
                "key TEXT PRIMARY KEY,"
                "value INTEGER NOT NULL DEFAULT 0)"))) {
            setLastError(q.lastError().text());
            return false;
        }
    }

    const bool haveCount = metaValue(db, QStringLiteral("entry_count")) >= 0;
    const bool capOk = hasTrigger(db, "share_entries_cap");

    // Usable index: skip all writes (missing count triggers are repaired later).
    if (haveCount && capOk)
        return true;

    if (!hasTrigger(db, "share_entries_cnt_ai")) {
        QSqlQuery q(db);
        q.exec(QStringLiteral(
            "CREATE TRIGGER share_entries_cnt_ai AFTER INSERT ON share_entries BEGIN "
            "UPDATE share_index_meta SET value = value + 1 WHERE key = 'entry_count'; "
            "END"));
    }
    if (!hasTrigger(db, "share_entries_cnt_ad")) {
        QSqlQuery q(db);
        q.exec(QStringLiteral(
            "CREATE TRIGGER share_entries_cnt_ad AFTER DELETE ON share_entries BEGIN "
            "UPDATE share_index_meta SET value = value - 1 WHERE key = 'entry_count'; "
            "END"));
    }
    if (!capOk) {
        QSqlQuery q(db);
        q.exec(QStringLiteral(
            "CREATE TRIGGER share_entries_cap AFTER INSERT ON share_entries "
            "WHEN COALESCE((SELECT value FROM share_index_meta WHERE key = 'cap_armed'), 1) != 0 "
            "AND (SELECT value FROM share_index_meta WHERE key = 'entry_count') > %1 "
            "BEGIN "
            "DELETE FROM share_entries WHERE id = ("
            "SELECT id FROM share_entries "
            "ORDER BY show_hits ASC, updated_at ASC, created_at ASC "
            "LIMIT 1);"
            "END").arg(kMaxEntries));
    }

    QString err;
    if (!haveCount) {
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("SELECT count(*) FROM share_entries")) || !q.next()) {
            setLastError(q.lastError().text());
            return false;
        }
        if (!upsertMeta(db, QStringLiteral("entry_count"), q.value(0).toLongLong(), &err)) {
            setLastError(err);
            return false;
        }
    }
    if (metaValue(db, QStringLiteral("cap_armed"), -1) < 0
            && !upsertMeta(db, QStringLiteral("cap_armed"), 1, &err)) {
        setLastError(err);
        return false;
    }
    return true;
}

void ShareIndex::setCapArmed(QSqlDatabase &db, bool armed)
{
    upsertMeta(db, QStringLiteral("cap_armed"), armed ? 1 : 0, nullptr);
}

void ShareIndex::pruneExcess(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "SELECT value FROM share_index_meta WHERE key = 'entry_count'"))
            || !q.next())
        return;

    const qint64 excess = q.value(0).toLongLong() - kMaxEntries;
    if (excess <= 0)
        return;

    q.prepare(QStringLiteral(
        "DELETE FROM share_entries WHERE id IN ("
        "SELECT id FROM share_entries "
        "ORDER BY show_hits ASC, updated_at ASC, created_at ASC "
        "LIMIT ?)"));
    q.addBindValue(excess);
    if (!q.exec())
        setLastError(q.lastError().text());
}

#endif
