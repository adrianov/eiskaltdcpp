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
#include <QAtomicInt>

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
    bool isOpen() const { return opened.loadAcquire() != 0; }

    void ingestList(const dcpp::UserPtr &user, const QString &listPath,
                    const QString &hubUrl, const QString &nick);

    /** Force re-index one list (ignores mtime skip); returns wall ms. CLI/bench. */
    qint64 forceIngestListMs(const dcpp::UserPtr &user, const QString &listPath,
                             const QString &hubUrl, const QString &nick);

    /** Index all *.xml / *.xml.bz2 under the FileLists directory (queued). */
    void ingestCachedLists();

    void upsertFromSearch(const QVariantMap &map);

    /** Block until all queued writes finish (CLI / tests). */
    void waitWritesIdle();

    QList<QVariantMap> search(const SearchFilter &filter);

    struct IndexStats {
        qint64 files = 0;
        qint64 dbBytes = 0;
    };
    IndexStats indexStats();
    /** True when cid has no rows, or listPath mtime/size differs from last ingest. */
    bool needsListIngest(const QString &cid, const QString &listPath = QString());
    /** Close this thread's QSqlDatabase before the worker exits. */
    void releaseThreadDb();

    static bool smokeCheck(QString *error = nullptr);
    static QString nowStamp();
    QString lastError() const;

#ifdef USE_QT_SQLITE
    friend bool shareIndexSmokeSearch(ShareIndex &idx, QSqlDatabase &db, QString *error);
    friend void shareIndexRunWriteWorker();
#endif

private:
    ShareIndex();
    ~ShareIndex() override;

#ifdef USE_QT_SQLITE
    bool ensureSchema(QSqlDatabase &db);
    bool ensureFts(QSqlDatabase &db);
    /** Per-thread connection; Qt forbids sharing QSqlDatabase across threads. */
    QSqlDatabase threadDb();
    void disconnectThreadDb();
    void setLastError(const QString &err);

    bool upsertRow(QSqlDatabase &db, const QVariantMap &row, int source);
    bool insertRow(QSqlDatabase &db, const QVariantMap &row, int source);
    bool prepareInsert(QSqlQuery &ins) const;
    bool bindInsert(QSqlQuery &ins, const QVariantMap &row, int source) const;
    QList<QVariantMap> searchFts(QSqlDatabase &db, const SearchFilter &filter);
    QList<QVariantMap> rowsFromQuery(QSqlQuery &q);
    QString filterSql(const SearchFilter &filter, QVariantList &binds) const;

    void walkListing(dcpp::DirectoryListing &listing,
                     dcpp::DirectoryListing::Directory *dir,
                     const QString &cid, const QString &hubUrl,
                     const QString &nick, const QString &hubName,
                     QList<QVariantMap> &out);

    void ingestListSync(const dcpp::UserPtr &user, const QString &listPath,
                        const QString &hubUrl, const QString &nick,
                        bool force = false);
    void ingestCachedListsSync();
    void upsertFromSearchSync(const QVariantMap &map);
    void drainWriteQueue();
    bool pendingListIngest() const;
    void requeueCachedIngest();
    void rememberListMeta(const QString &cid, const QString &listPath, int rowCount);

    QString dbFile;
#endif
    QAtomicInt opened;
    /** One-time schema/FTS open. */
    mutable QMutex openMutex;
    /** Serializes QSqlDatabase registry add/remove across threads. */
    mutable QMutex connMutex;
    mutable QMutex errorMutex;
    QString lastSqlError;
};
