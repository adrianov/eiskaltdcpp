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
#include <QHash>
#include <QSet>

#include <memory>

#ifdef USE_QT_SQLITE
#include "ShareIndexDb.h"
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
    /** Open schema on the write worker (never block the UI). */
    void openAsync();
    bool isOpen() const { return opened.loadAcquire() != 0; }

    void ingestList(const dcpp::UserPtr &user, const QString &listPath,
                    const QString &hubUrl, const QString &nick);
    /** Match queued TTHs against saved lists owned by newly online users. */
    void matchQueue(const dcpp::UserList &users);
    /** Drop stale (cid, tth) rows after File Not Available. */
    void removeTth(const QString &cid, const QString &tth);

    /** Force re-index one list (ignores mtime skip); returns wall ms. CLI/bench. */
    qint64 forceIngestListMs(const dcpp::UserPtr &user, const QString &listPath,
                             const QString &hubUrl, const QString &nick);

    void upsertFromSearch(const QVariantMap &map);

    /** Block until all queued writes finish (CLI / tests). */
    void waitWritesIdle();
    /** Drop pending jobs and wait for the write worker to exit (app quit). */
    void stopWrites();

    QList<QVariantMap> search(const SearchFilter &filter);

    /** Fast index HUD: entry_count meta + on-disk DB size (no table scan). */
    struct IndexStats {
        qint64 files = 0;   // share_index_meta.entry_count (files + dirs)
        qint64 dbBytes = 0;
    };
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
    friend bool shareIndexSmokeMigrate(const QString &path, QString *error);
    friend void shareIndexRunWriteWorker();
#endif

private:
    ShareIndex();
    ~ShareIndex() override;

#ifdef USE_QT_SQLITE
    bool ensureSchema(duckdb::Connection &con, const std::string &prefix = std::string());
    bool ensureCap(duckdb::Connection &con);
    /** Replace a legacy flat share_entries DB with an empty normalized schema. */
    bool compactLegacyDb();
    duckdb::Connection *threadConn();
    void disconnectThreadDb();
    void setLastError(const QString &err);

    bool upsertRow(duckdb::Connection &con, const QVariantMap &row, int source);
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
    /** Chunked DELETE+Appender for one file list; returns false on abort/error. */
    bool writeListRows(const QString &cid, const QList<QVariantMap> &rows);
    void upsertFromSearchBatchSync(const QList<QVariantMap> &maps);
    void pruneExcess(duckdb::Connection &con);
    bool removeOrphans(duckdb::Connection &con);
    void refreshEntryCount(duckdb::Connection &con);
    void reclaimFreePages(duckdb::Connection &con);
    void drainWriteQueue();
    void rememberListMeta(const QString &cid, const QString &listPath, int rowCount);

    QString dbFile;
    std::unique_ptr<duckdb::DuckDB> duck;
    QHash<quintptr, std::shared_ptr<duckdb::Connection>> threadConns;
#endif
    QAtomicInt opened;
    /** One-time schema open. */
    mutable QMutex openMutex;
    /** Serializes per-thread connection map. */
    mutable QMutex connMutex;
    mutable QMutex errorMutex;
    QString lastSqlError;
};
