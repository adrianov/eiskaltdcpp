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

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QList>
#include <QMutex>
#include <QAtomicInt>
#include <QHash>
#include <QSet>

#include <memory>

#include "shareindex/ShareIndexModels.h"
#ifdef USE_QT_SQLITE
#include "shareindex/ShareIndexStore.h"
#endif

#include "dcpp/stdinc.h"
#include "dcpp/User.h"
#include "dcpp/DirectoryListing.h"
#include "dcpp/Singleton.h"

/** Persistent index of remote share entries from file lists and hub search (DuckDB). */
class ShareIndex : public QObject, public dcpp::Singleton<ShareIndex>
{
    Q_OBJECT
    friend class dcpp::Singleton<ShareIndex>;

public:
    enum Source {
        SourceFileList = ShareIndexModels::SourceFileList,
        SourceHubSearch = ShareIndexModels::SourceHubSearch
    };
    using SearchFilter = ShareIndexModels::SearchFilter;
    using IndexUser = ShareIndexModels::IndexUser;
    using MediaInfo = ShareIndexModels::MediaInfo;
    using IndexStats = ShareIndexModels::IndexStats;

    /** Uppercase file suffix without dot; empty for directories / no suffix. */
    static QString fileExt(const QString &name, bool isDir);

    void open();
    /** Open schema on the write worker (never block the UI). */
    void openAsync();
    bool isOpen() const
    {
#ifdef USE_QT_SQLITE
        return store.isOpen();
#else
        return opened.loadAcquire() != 0;
#endif
    }

    void ingestList(const dcpp::UserPtr &user, const QString &listPath,
                    const QString &hubUrl, const QString &nick);
    /** Match queued TTHs against indexed locations for these online users. */
    void matchQueue(const dcpp::UserList &users);
    /** Drop stale (cid, tth) rows after File Not Available. */
    void removeTth(const QString &cid, const QString &tth);
    /** Drop all indexed data for a silent/unreachable peer (cid). */
    void removeUser(const QString &cid);

    /** Force re-index one list (ignores mtime skip); returns wall ms. CLI/bench. */
    qint64 forceIngestListMs(const dcpp::UserPtr &user, const QString &listPath,
                             const QString &hubUrl, const QString &nick);

    void upsertFromSearch(const QVariantMap &map);

    /** Block until all queued writes finish (CLI / tests). */
    void waitWritesIdle();
    /** Drop pending jobs and wait for the write worker to exit (app quit). */
    void stopWrites();

    QList<QVariantMap> search(const SearchFilter &filter);
    /** Interrupt an in-flight local search (new Search / Stop). */
    void cancelSearch();

    QHash<QString, QList<IndexUser>> usersByTth(const QStringList &tths, qint64 size = 0,
                                                 int limitPerTth = 64);
    QHash<QString, MediaInfo> mediaByTth(const QStringList &tths);
    /** Fill empty media fields on existing share_files rows (from a loaded listing). */
    void upsertMedia(const QHash<QString, MediaInfo> &media);

    IndexStats indexStats();
    /** True when cid has no rows, or listPath mtime/size differs from last ingest. */
    bool needsListIngest(const QString &cid, const QString &listPath = QString());
    /** Close this thread's DuckDB connection before the worker exits. */
    void releaseThreadDb();

    static bool smokeCheck(QString *error = nullptr);
    static QString nowStamp();
    QString lastError() const;

#ifdef USE_QT_SQLITE
    friend bool shareIndexSmokeSearch(ShareIndex &idx, duckdb::Connection &con, QString *error);
    friend bool shareIndexSmokeUsers(ShareIndex &idx, duckdb::Connection &con, QString *error);
    friend bool shareIndexSmokeMigrate(const QString &path, QString *error);
    friend bool shareIndexSmokeWrites(ShareIndex &idx, duckdb::Connection &con, QString *error);
    friend void shareIndexRunWriteWorker();
#endif

private:
    ShareIndex();
    ~ShareIndex() override;

#ifdef USE_QT_SQLITE
    bool ensureSchema(duckdb::Connection &con, const std::string &prefix = std::string());
    bool ensureCap(duckdb::Connection &con);
    bool compactLegacyDb();
    duckdb::Connection *threadConn();
    void disconnectThreadDb();
    void setLastError(const QString &err);

    bool upsertRow(duckdb::Connection &con, const QVariantMap &row, int source);
    qint64 ensureUserId(duckdb::Connection &con, const QVariantMap &row, const QString &stamp);
    qint64 ensureFileId(duckdb::Connection &con, const QString &tth, qint64 size,
                        const QString &name, const QString &path, const QString &ext,
                        QString *cName, QString *cPath, QString *cExt,
                        QString *cNameCf, QString *cPathCf);
    bool appendListRows(duckdb::Connection &con, const QList<QVariantMap> &rows);
    QList<QVariantMap> searchFts(duckdb::Connection &con, const SearchFilter &filter);
    QList<QVariantMap> rowsFromResult(duckdb::MaterializedQueryResult &res);
    QString filterSql(const SearchFilter &filter, duckdb::vector<duckdb::Value> &binds) const;

    void walkListing(dcpp::DirectoryListing &listing,
                     dcpp::DirectoryListing::Directory *dir,
                     const QString &cid, const QString &hubUrl,
                     const QString &nick, const QString &hubName,
                     const QString &ip, QSet<QString> &seen,
                     QList<QVariantMap> &out);

    void ingestListSync(const dcpp::UserPtr &user, const QString &listPath,
                        const QString &hubUrl, const QString &nick,
                        bool force = false);
    void matchQueueSync(const dcpp::UserList &users);
    void removeTthSync(const QString &cid, const QString &tth);
    void removeUserSync(const QString &cid);
    bool writeListRows(const QString &cid, const QList<QVariantMap> &rows);
    void upsertFromSearchBatchSync(const QList<QVariantMap> &maps);
    void upsertMediaSync(const QHash<QString, MediaInfo> &media);
    bool removeOrphans(duckdb::Connection &con);
    bool refreshEntryCount(duckdb::Connection &con);
    void reclaimFreePages(duckdb::Connection &con);
    void drainWriteQueue();
    void closeDb();
    void wipeDbFiles();
    void recoverDb();
    bool finishOpen();
    void rememberListMeta(const QString &cid, const QString &listPath, int rowCount);

    ShareIndexStore store;
    duckdb::Connection *activeSearchCon = nullptr;
    QAtomicInt searchEpoch;
    mutable QMutex searchMu;
#else
    QAtomicInt opened;
#endif
    mutable QMutex errorMutex;
    QString lastSqlError;
};
