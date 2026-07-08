/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"

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
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", (isUpload? "FinishedUploadsLoader" : "FinishedDownloadsLoader"));
    db.setDatabaseName(db_file);

    bool db_opened = db.open();

    if (!db_opened)
        return;

    QSqlQuery q(db);

    q.exec("SELECT * FROM files LIMIT 0, 500;"); // temporary limitation

    VarMap params;

    model->beginBulkLoad();

    while (q.next()){
        int i = 0;

        params["FNAME"] = q.value(i++);
        params["TIME"]  = q.value(i++);
        params["PATH"]  = q.value(i++);
        params["USERS"] = q.value(i++);
        params["TR"]    = q.value(i++);
        params["SPEED"] = q.value(i++);
        params["CRC32"] = q.value(i++);
        params["TARGET"]= q.value(i++);
        params["ELAP"]  = q.value(i++);
        params["FULL"]  = q.value(i++);

        if (!showDownloadParams(params))
            continue;

        if (!isUpload && !Util::fileExists(_tq(params["TARGET"].toString()))) {
            QSqlQuery del(db);
            del.prepare("DELETE FROM files WHERE TARGET = ?;");
            del.bindValue(0, params["TARGET"]);
            del.exec();
            continue;
        }

        emit coreAddedFile(params);
    }

    params.clear();

    q.exec("SELECT * FROM users LIMIT 0, 500;");

    while (q.next()){
        int i = 0;

        params["NICK"] = q.value(i++);
        params["TIME"]  = q.value(i++);
        params["FILES"]  = q.value(i++);
        params["TR"]    = q.value(i++);
        params["SPEED"] = q.value(i++);
        params["CID"] = q.value(i++);
        params["ELAP"]  = q.value(i++);
        params["FULL"]  = q.value(i++);

        emit coreAddedUser(params);
    }

    model->endBulkLoad();

    db.close();
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::getParams(const FinishedFileItemPtr& item, const string& file, VarMap &params)
{
    QString nicks = "";

    params["FNAME"] = _q(file).split(QDir::separator()).last();
    params["TIME"]  = _q(Util::formatTime("%Y-%m-%d %H:%M:%S", item->getTime()));
    params["PATH"]  = _q(Util::getFilePath(file));

    for (const auto &user : item->getUsers()) {
        nicks += WulforUtil::getInstance()->getNicks(user.user->getCID()) + " ";
    }

    params["USERS"] = nicks;
    params["TR"]    = (qlonglong)item->getTransferred();
    params["SPEED"] = (qlonglong)item->getAverageSpeed();
    params["CRC32"] = item->getCrc32Checked();
    params["TARGET"]= _q(file);
    params["ELAP"]  = (qlonglong)item->getMilliSeconds();
    params["FULL"]  = item->isFull();

#ifdef USE_QT_SQLITE
    if (!db_opened)
        return;

    QSqlQuery q(db);
    q.prepare("REPLACE INTO files "
              "(FNAME, TIME, PATH, USERS, TR, SPEED, CRC32, TARGET, ELAP, FULL) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    q.bindValue(0, params["FNAME"]);
    q.bindValue(1, params["TIME"]);
    q.bindValue(2, params["PATH"]);
    q.bindValue(3, params["USERS"]);
    q.bindValue(4, params["TR"]);
    q.bindValue(5, params["SPEED"]);
    q.bindValue(6, params["CRC32"]);
    q.bindValue(7, params["TARGET"]);
    q.bindValue(8, params["ELAP"]);
    q.bindValue(9, params["FULL"]);

    q.exec();
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::getParams(const FinishedUserItemPtr& item, const UserPtr& user, VarMap &params)
{
    QString files = "";

    params["TIME"]  = _q(Util::formatTime("%Y-%m-%d %H:%M:%S", item->getTime()));
    params["NICK"]  = WulforUtil::getInstance()->getNicks(user->getCID());

    for (const auto &file: item->getFiles()) {
            files += _q(file) + " ";
    }

    params["FILES"] = files;
    params["TR"]    = (qlonglong)item->getTransferred();
    params["SPEED"] = (qlonglong)item->getAverageSpeed();
    params["CID"]   = _q(user->getCID().toBase32());
    params["ELAP"]  = (qlonglong)item->getMilliSeconds();
    params["FULL"]  = true;

#ifdef USE_QT_SQLITE
    if (!db_opened)
        return;

    QSqlQuery q(db);
    q.prepare("REPLACE INTO users "
              "(NICK, TIME, FILES, TR, SPEED, CID, ELAP, FULL)"
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    q.bindValue(0, params["NICK"]);
    q.bindValue(1, params["TIME"]);
    q.bindValue(2, params["FILES"]);
    q.bindValue(3, params["TR"]);
    q.bindValue(4, params["SPEED"]);
    q.bindValue(5, params["CID"]);
    q.bindValue(6, params["ELAP"]);
    q.bindValue(7, params["FULL"]);

    q.exec();

#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::removeFileFromDB(const QString &target)
{
#ifdef USE_QT_SQLITE
    if (!db_opened || target.isEmpty())
        return;

    QSqlQuery q(db);
    q.prepare("DELETE FROM files WHERE TARGET = ?;");
    q.bindValue(0, target);
    q.exec();
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::pruneMissingFiles()
{
    const QStringList targets = model->fileTargets();
    for (const QString &qtarget : targets) {
        if (Util::fileExists(_tq(qtarget)))
            continue;

        try {
            FinishedManager::getInstance()->remove(false, _tq(qtarget));
        } catch (const std::exception&) {}

        removeFileFromDB(qtarget);
        model->remFile(qtarget);
    }
}

template void FinishedTransfers<true>::loadList();
template void FinishedTransfers<false>::loadList();
template void FinishedTransfers<true>::loadListFromDB();
template void FinishedTransfers<false>::loadListFromDB();
template void FinishedTransfers<true>::getParams(const FinishedFileItemPtr&, const string&, VarMap&);
template void FinishedTransfers<false>::getParams(const FinishedFileItemPtr&, const string&, VarMap&);
template void FinishedTransfers<true>::getParams(const FinishedUserItemPtr&, const UserPtr&, VarMap&);
template void FinishedTransfers<false>::getParams(const FinishedUserItemPtr&, const UserPtr&, VarMap&);
template void FinishedTransfers<true>::removeFileFromDB(const QString&);
template void FinishedTransfers<false>::removeFileFromDB(const QString&);
template void FinishedTransfers<true>::pruneMissingFiles();
template void FinishedTransfers<false>::pruneMissingFiles();
