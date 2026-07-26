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
}

template <bool isUpload>
void FinishedTransfers<isUpload>::persistFile(const VarMap &params)
{
#ifdef USE_QT_SQLITE
    if (!db_opened || params["TARGET"].toString().isEmpty())
        return;

    QSqlQuery q(db);
    q.prepare("REPLACE INTO files "
              "(TARGET, FNAME, TIME, PATH, USERS, TR, SPEED, CRC32, ELAP, FULL) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    q.bindValue(0, params["TARGET"]);
    q.bindValue(1, params["FNAME"]);
    q.bindValue(2, params["TIME"]);
    q.bindValue(3, params["PATH"]);
    q.bindValue(4, params["USERS"]);
    q.bindValue(5, params["TR"]);
    q.bindValue(6, params["SPEED"]);
    q.bindValue(7, params["CRC32"]);
    q.bindValue(8, params["ELAP"]);
    q.bindValue(9, params["FULL"]);
    q.exec();
#else
    Q_UNUSED(params)
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::persistUser(const VarMap &params)
{
#ifdef USE_QT_SQLITE
    if (!db_opened || params["CID"].toString().isEmpty())
        return;

    QSqlQuery q(db);
    q.prepare("REPLACE INTO users "
              "(CID, NICK, TIME, FILES, TR, SPEED, ELAP, FULL) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    q.bindValue(0, params["CID"]);
    q.bindValue(1, params["NICK"]);
    q.bindValue(2, params["TIME"]);
    q.bindValue(3, params["FILES"]);
    q.bindValue(4, params["TR"]);
    q.bindValue(5, params["SPEED"]);
    q.bindValue(6, params["ELAP"]);
    q.bindValue(7, params["FULL"]);
    q.exec();
#else
    Q_UNUSED(params)
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::removeFileDB(const QString &target)
{
#ifdef USE_QT_SQLITE
    if (!db_opened || target.isEmpty())
        return;

    QSqlQuery q(db);
    q.prepare("DELETE FROM files WHERE TARGET = ?;");
    q.bindValue(0, target);
    q.exec();
#else
    Q_UNUSED(target)
#endif
}

template void FinishedTransfers<true>::getParams(const FinishedFileItemPtr&, const string&, VarMap&);
template void FinishedTransfers<false>::getParams(const FinishedFileItemPtr&, const string&, VarMap&);
template void FinishedTransfers<true>::getParams(const FinishedUserItemPtr&, const UserPtr&, VarMap&);
template void FinishedTransfers<false>::getParams(const FinishedUserItemPtr&, const UserPtr&, VarMap&);
template void FinishedTransfers<true>::persistFile(const VarMap&);
template void FinishedTransfers<false>::persistFile(const VarMap&);
template void FinishedTransfers<true>::persistUser(const VarMap&);
template void FinishedTransfers<false>::persistUser(const VarMap&);
template void FinishedTransfers<true>::removeFileDB(const QString&);
template void FinishedTransfers<false>::removeFileDB(const QString&);
