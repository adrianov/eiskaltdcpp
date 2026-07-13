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

#include <QElapsedTimer>

/** TTH → users lookup used by Download Queue User column. */
bool shareIndexSmokeUsers(ShareIndex &idx, duckdb::Connection &con, QString *error)
{
    const QString tth = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ23456C");
    const QString other = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ23456D");
    const qint64 size = 42;

    QVariantMap a;
    a["cid"] = "CIDALICE";
    a["hub_url"] = "adc://hub";
    a["tth"] = tth;
    a["path"] = "A\\";
    a["name"] = "queue-user-a.bin";
    a["is_dir"] = false;
    a["size"] = size;
    a["nick"] = "Alice";
    a["hub_name"] = "TestHub";
    if (!idx.upsertRow(con, a, ShareIndex::SourceFileList)) {
        if (error)
            *error = "insert Alice holder";
        return false;
    }

    QVariantMap b;
    b["cid"] = "CIDBOB";
    b["hub_url"] = "adc://hub2";
    b["tth"] = tth;
    b["path"] = "B\\";
    b["name"] = "queue-user-b.bin";
    b["is_dir"] = false;
    b["size"] = size;
    b["nick"] = "Bob";
    b["hub_name"] = "TestHub2";
    if (!idx.upsertRow(con, b, ShareIndex::SourceHubSearch)) {
        if (error)
            *error = "insert Bob holder";
        return false;
    }

    QVariantMap c;
    c["cid"] = "CIDCAROL";
    c["hub_url"] = "adc://hub3";
    c["tth"] = other;
    c["path"] = "C\\";
    c["name"] = "queue-user-c.bin";
    c["is_dir"] = false;
    c["size"] = size + 1;
    c["nick"] = "Carol";
    c["hub_name"] = "TestHub3";
    if (!idx.upsertRow(con, c, ShareIndex::SourceFileList)) {
        if (error)
            *error = "insert Carol holder";
        return false;
    }

    QElapsedTimer timer;
    timer.start();
    const auto byTth = idx.usersByTth(QStringList{tth}, size);
    const qint64 ms = timer.elapsed();

    const auto users = byTth.value(tth);
    if (users.size() != 2) {
        if (error)
            *error = QString("usersByTth expected 2 got %1").arg(users.size());
        return false;
    }
    QStringList nicks;
    for (const auto &u : users)
        nicks << u.nick;
    nicks.sort();
    if (nicks.join(',') != QStringLiteral("Alice,Bob")) {
        if (error)
            *error = QString("usersByTth nicks: %1").arg(nicks.join(','));
        return false;
    }
    if (ms > 250) {
        if (error)
            *error = QString("usersByTth too slow: %1 ms").arg(ms);
        return false;
    }

    // Size filter must not return the other TTH's holders.
    if (!idx.usersByTth(QStringList{tth}, size + 1).value(tth).isEmpty()) {
        if (error)
            *error = "usersByTth size filter leak";
        return false;
    }
    if (idx.usersByTth(QStringList{other}, size + 1).value(other).size() != 1) {
        if (error)
            *error = "usersByTth other TTH";
        return false;
    }

    const auto batch = idx.usersByTth(QStringList{tth, other}, 0);
    if (batch.value(tth).size() != 2 || batch.value(other).size() != 1) {
        if (error)
            *error = "usersByTth batch";
        return false;
    }
    return true;
}

#endif
