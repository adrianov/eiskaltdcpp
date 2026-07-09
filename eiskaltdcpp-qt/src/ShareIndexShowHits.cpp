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

    if (!db.transaction())
        return;

    QSqlQuery q(db);
    q.prepare("UPDATE share_entries SET show_hits = show_hits + 1 WHERE id = ?");
    for (qint64 id : ids) {
        if (id <= 0)
            continue;
        q.addBindValue(id);
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
