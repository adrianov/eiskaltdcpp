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

#include "FinishedTransfers.h"
#include "sharebrowser/AsyncRunner.h"

#ifdef USE_QT_SQLITE
#include <QtSql>
#endif

template <bool isUpload>
void FinishedTransfers<isUpload>::loadList()
{
    VarMap params;
    StringList removeFileLists;

    {
        auto lock = FinishedManager::getInstance()->lock();
        const FinishedManager::MapByFile &list = FinishedManager::getInstance()->getMapByFile(isUpload);
        const FinishedManager::MapByUser &user = FinishedManager::getInstance()->getMapByUser(isUpload);

        model->beginBulkLoad();

        for (auto it = list.begin(); it != list.end(); ++it) {
            if (!isUpload && isFileListPath(it->first)) {
                removeFileLists.push_back(it->first);
                continue;
            }

            if (!showDownload(it->first, it->second))
                continue;

            params.clear();
            getParams(it->second, it->first, params);
            model->addFile(params);
        }

        for (auto uit = user.begin(); uit != user.end(); ++uit) {
            params.clear();
            getParams(uit->second, uit->first, params);
            model->addUser(params);
        }

        model->endBulkLoad();
    }

    for (const auto &file : removeFileLists) {
        try {
            FinishedManager::getInstance()->remove(false, file);
        } catch (const std::exception&) {}
    }

    AsyncRunner *runner = new AsyncRunner(this);

    runner->setRunFunction([this]() { this->loadListFromDB(); });
    connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));

    runner->start();
}

template <bool isUpload>
void FinishedTransfers<isUpload>::loadListFromDB()
{
#ifdef USE_QT_SQLITE
    const QString conn = isUpload ? QStringLiteral("FinishedUploadsLoader")
                                  : QStringLiteral("FinishedDownloadsLoader");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
        db.setDatabaseName(db_file);
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));

        if (db.open()) {
            QSqlQuery q(db);
            VarMap params;
            // TIME is "%Y-%m-%d %H:%M:%S" — lexicographic ORDER BY matches chronology.
            static const int keep = 500;
            const QString keepStr = QString::number(keep);

            // Cap the DB to the newest rows; unordered LIMIT used to restore the oldest.
            q.exec(QStringLiteral(
                "DELETE FROM files WHERE rowid NOT IN ("
                "SELECT rowid FROM ("
                "SELECT rowid FROM files ORDER BY TIME DESC LIMIT %1));").arg(keepStr));
            q.exec(QStringLiteral(
                "DELETE FROM users WHERE rowid NOT IN ("
                "SELECT rowid FROM ("
                "SELECT rowid FROM users ORDER BY TIME DESC LIMIT %1));").arg(keepStr));

            emit coreBeginBulkLoad();

            q.exec(QStringLiteral(
                "SELECT TARGET, FNAME, TIME, PATH, USERS, TR, SPEED, CRC32, ELAP, FULL "
                "FROM files ORDER BY TIME DESC LIMIT %1;").arg(keepStr));
            while (q.next()) {
                int i = 0;
                params["TARGET"]= q.value(i++);
                params["FNAME"] = q.value(i++);
                params["TIME"]  = q.value(i++);
                params["PATH"]  = q.value(i++);
                params["USERS"] = q.value(i++);
                params["TR"]    = q.value(i++);
                params["SPEED"] = q.value(i++);
                params["CRC32"] = q.value(i++);
                params["ELAP"]  = q.value(i++);
                params["FULL"]  = q.value(i++);

                if (!showDownloadParams(params))
                    continue;

                // Keep rows even if the target is briefly missing (Complete persists
                // before FileMover finishes; pruneMissingFiles cleans up later).
                emit coreLoadedFile(params);
            }

            params.clear();
            q.exec(QStringLiteral(
                "SELECT CID, NICK, TIME, FILES, TR, SPEED, ELAP, FULL "
                "FROM users ORDER BY TIME DESC LIMIT %1;").arg(keepStr));
            while (q.next()) {
                int i = 0;
                params["CID"]  = q.value(i++);
                params["NICK"] = q.value(i++);
                params["TIME"]  = q.value(i++);
                params["FILES"]  = q.value(i++);
                params["TR"]    = q.value(i++);
                params["SPEED"] = q.value(i++);
                params["ELAP"]  = q.value(i++);
                params["FULL"]  = q.value(i++);
                emit coreLoadedUser(params);
            }

            emit coreEndBulkLoad();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(conn);
#endif
}

template void FinishedTransfers<true>::loadList();
template void FinishedTransfers<false>::loadList();
template void FinishedTransfers<true>::loadListFromDB();
template void FinishedTransfers<false>::loadListFromDB();
