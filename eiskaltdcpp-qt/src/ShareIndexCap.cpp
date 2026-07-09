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

} // namespace

bool ShareIndex::ensureCap(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS share_index_meta ("
            "key TEXT PRIMARY KEY,"
            "value INTEGER NOT NULL DEFAULT 0)"))) {
        setLastError(q.lastError().text());
        return false;
    }

    // Recreate so trigger body updates apply across versions.
    q.exec(QStringLiteral("DROP TRIGGER IF EXISTS share_entries_cnt_ai"));
    q.exec(QStringLiteral("DROP TRIGGER IF EXISTS share_entries_cnt_ad"));
    q.exec(QStringLiteral("DROP TRIGGER IF EXISTS share_entries_cap"));

    // O(1) WHEN checks (count(*) at 5M would stall every INSERT).
    q.exec(QStringLiteral(
        "CREATE TRIGGER share_entries_cnt_ai AFTER INSERT ON share_entries BEGIN "
        "UPDATE share_index_meta SET value = value + 1 WHERE key = 'entry_count'; "
        "END"));
    q.exec(QStringLiteral(
        "CREATE TRIGGER share_entries_cnt_ad AFTER DELETE ON share_entries BEGIN "
        "UPDATE share_index_meta SET value = value - 1 WHERE key = 'entry_count'; "
        "END"));

    // Evict lowest show_hits, then oldest updated_at / created_at.
    // cap_armed=0 during bulk ingest; recursive_triggers keep FTS/count in sync.
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

    QString err;
    if (!q.exec(QStringLiteral("SELECT count(*) FROM share_entries")) || !q.next()) {
        setLastError(q.lastError().text());
        return false;
    }
    if (!upsertMeta(db, QStringLiteral("entry_count"), q.value(0).toLongLong(), &err)
            || !upsertMeta(db, QStringLiteral("cap_armed"), 1, &err)) {
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
