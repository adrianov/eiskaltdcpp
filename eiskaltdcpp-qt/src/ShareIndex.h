/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QList>
#include <QMutex>

#ifdef USE_QT_SQLITE
#include <QtSql>
#endif

#include "dcpp/stdinc.h"
#include "dcpp/User.h"
#include "dcpp/DirectoryListing.h"
#include "dcpp/Singleton.h"

/** Persistent index of remote share entries from file lists and hub search. */
class ShareIndex : public QObject, public dcpp::Singleton<ShareIndex>
{
    Q_OBJECT
    friend class dcpp::Singleton<ShareIndex>;

public:
    enum Source {
        SourceFileList = 1,
        SourceHubSearch = 2
    };

    struct SearchFilter {
        QStringList terms;
        QStringList extensions; // uppercase, no dot; empty = any
        bool isHash = false;
        bool dirsOnly = false;
        bool filesOnly = false;
        qint64 size = 0;
        int sizeMode = 0;
        int limit = 500;
    };

    /** Uppercase file suffix without dot; empty for directories / no suffix. */
    static QString fileExt(const QString &name, bool isDir);

    void open();
    bool isOpen() const { return opened; }

    void ingestList(const dcpp::UserPtr &user, const QString &listPath,
                    const QString &hubUrl, const QString &nick);

    void upsertFromSearch(const QVariantMap &map);

    QList<QVariantMap> search(const SearchFilter &filter);

    static bool smokeCheck(QString *error = nullptr);
    static QString nowStamp();
    QString lastError() const { return lastSqlError; }

private:
    ShareIndex();
    ~ShareIndex() override;

#ifdef USE_QT_SQLITE
    bool ensureSchema(QSqlDatabase &db);
    bool ensureFts(QSqlDatabase &db);
    QSqlDatabase connectDb(const QString &connName);
    void disconnectDb(const QString &connName);

    bool upsertRow(QSqlDatabase &db, const QVariantMap &row, int source);
    QList<QVariantMap> searchFts(QSqlDatabase &db, const SearchFilter &filter);
    QList<QVariantMap> searchLike(QSqlDatabase &db, const SearchFilter &filter);
    QList<QVariantMap> rowsFromQuery(QSqlQuery &q);
    QString filterSql(const SearchFilter &filter, QVariantList &binds) const;

    void walkListing(dcpp::DirectoryListing &listing,
                     dcpp::DirectoryListing::Directory *dir,
                     const QString &cid, const QString &hubUrl,
                     const QString &nick, const QString &hubName,
                     QList<QVariantMap> &out);

    QString dbFile;
    bool ftsReady;
#endif
    bool opened;
    mutable QMutex mutex;
    QString lastSqlError;
};
