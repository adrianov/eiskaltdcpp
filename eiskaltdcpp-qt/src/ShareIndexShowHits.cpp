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
#include <QVariant>

namespace {

/** Stay under SQLite's default bind-variable limit. */
const int kIdChunk = 400;

} // namespace

void ShareIndex::recordSearchShowsSync(const QList<qint64> &ids)
{
    if (ids.isEmpty())
        return;

    open();
    if (!isOpen())
        return;

    QSqlDatabase db = threadDb();
    if (!db.isOpen())
        return;

    QList<qint64> valid;
    valid.reserve(ids.size());
    for (qint64 id : ids) {
        if (id > 0)
            valid.append(id);
    }
    if (valid.isEmpty())
        return;

    if (!db.transaction())
        return;

    QSqlQuery q(db);
    for (int offset = 0; offset < valid.size(); offset += kIdChunk) {
        const int n = qMin(kIdChunk, valid.size() - offset);
        QStringList marks;
        marks.reserve(n);
        for (int i = 0; i < n; ++i)
            marks.append(QStringLiteral("?"));

        q.prepare(QStringLiteral(
                      "UPDATE share_entries SET show_hits = show_hits + 1 "
                      "WHERE id IN (%1)")
                      .arg(marks.join(QLatin1Char(','))));
        for (int i = 0; i < n; ++i)
            q.addBindValue(valid.at(offset + i));
        if (!q.exec()) {
            setLastError(q.lastError().text());
            db.rollback();
            return;
        }
        q.finish();
    }
    if (!db.commit())
        db.rollback();
}

#endif
