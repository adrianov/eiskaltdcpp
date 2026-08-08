/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"
#include "ShareIndexQueueCore.h"

#ifdef USE_QT_SQLITE

#include <QSet>

using namespace ShareIndexWriteQueue;

namespace {

std::string bindMarks(size_t count)
{
    std::string sql;
    for (size_t i = 0; i < count; ++i)
        sql += i ? ",?" : "?";
    return sql;
}

} // namespace

void ShareIndex::upsertMedia(const QHash<QString, MediaInfo> &media)
{
    if (media.isEmpty())
        return;
    WriteJob job;
    job.kind = UpsertMedia;
    job.media = media;
    enqueueWrite(job);
}

void ShareIndex::upsertMediaSync(const QHash<QString, MediaInfo> &media)
{
    if (media.isEmpty() || isStopping())
        return;

    open();
    duckdb::Connection *con = threadConn();
    if (!con)
        return;

    if (!ShareIndexDb::execOk(*con, "BEGIN TRANSACTION"))
        return;

    // Only fill empty columns; bind NULL for missing fields (never write 0/'').
    const char *sql =
        "UPDATE share_files SET "
        "bitrate = CASE WHEN ? IS NOT NULL AND coalesce(bitrate,0)=0 THEN ? ELSE bitrate END, "
        "resolution = CASE WHEN ? IS NOT NULL AND coalesce(resolution,'')='' THEN ? ELSE resolution END, "
        "video = CASE WHEN ? IS NOT NULL AND coalesce(video,'')='' THEN ? ELSE video END, "
        "audio = CASE WHEN ? IS NOT NULL AND coalesce(audio,'')='' THEN ? ELSE audio END "
        "WHERE tth = ?";

    for (auto it = media.constBegin(); it != media.constEnd(); ++it) {
        if (isStopping() || it.key().isEmpty() || it->isEmpty())
            continue;
        const duckdb::Value br = it->bitrate > 0
                ? duckdb::Value::INTEGER(it->bitrate) : ShareIndexDb::nullVal();
        const duckdb::Value res = it->resolution.isEmpty()
                ? ShareIndexDb::nullVal() : ShareIndexDb::strVal(it->resolution);
        const duckdb::Value video = it->video.isEmpty()
                ? ShareIndexDb::nullVal() : ShareIndexDb::strVal(it->video);
        const duckdb::Value audio = it->audio.isEmpty()
                ? ShareIndexDb::nullVal() : ShareIndexDb::strVal(it->audio);
        duckdb::vector<duckdb::Value> binds;
        binds.push_back(br);
        binds.push_back(br);
        binds.push_back(res);
        binds.push_back(res);
        binds.push_back(video);
        binds.push_back(video);
        binds.push_back(audio);
        binds.push_back(audio);
        binds.push_back(ShareIndexDb::strVal(it.key()));
        QString err;
        if (!ShareIndexDb::queryMat(*con, sql, binds, &err)) {
            ShareIndexDb::execOk(*con, "ROLLBACK");
            setLastError(err);
            return;
        }
    }

    if (!ShareIndexDb::execOk(*con, "COMMIT")) {
        setLastError(QStringLiteral("media upsert commit failed"));
        return;
    }
    setLastError(QString());
}

QHash<QString, ShareIndex::MediaInfo>
ShareIndex::mediaByTth(const QStringList &tths)
{
    QHash<QString, MediaInfo> out;
    if (tths.isEmpty() || !isOpen())
        return out;

    open();
    duckdb::Connection *con = threadConn();
    if (!con)
        return out;

    QStringList unique;
    QSet<QString> seen;
    for (const QString &t : tths) {
        if (t.isEmpty() || seen.contains(t))
            continue;
        seen.insert(t);
        unique << t;
    }
    if (unique.isEmpty())
        return out;

    const size_t n = size_t(unique.size());
    std::string sql =
        "SELECT tth, coalesce(bitrate,0), coalesce(resolution,''), "
        "coalesce(video,''), coalesce(audio,'') FROM share_files "
        "WHERE tth IN (" + bindMarks(n) + ") AND "
        "(coalesce(bitrate,0)>0 OR coalesce(resolution,'')!='' "
        "OR coalesce(video,'')!='' OR coalesce(audio,'')!='')";
    duckdb::vector<duckdb::Value> binds;
    for (const QString &t : unique)
        binds.push_back(ShareIndexDb::strVal(t));

    QString error;
    auto result = ShareIndexDb::queryMat(*con, sql, binds, &error);
    if (!result) {
        setLastError(error);
        return out;
    }
    setLastError(QString());

    for (idx_t row = 0; row < result->RowCount(); ++row) {
        const QString tth = ShareIndexDb::qstr(result->GetValue(0, row));
        if (tth.isEmpty())
            continue;
        MediaInfo m;
        m.bitrate = int(ShareIndexDb::qi64(result->GetValue(1, row)));
        m.resolution = ShareIndexDb::qstr(result->GetValue(2, row));
        m.video = ShareIndexDb::qstr(result->GetValue(3, row));
        m.audio = ShareIndexDb::qstr(result->GetValue(4, row));
        if (!m.isEmpty())
            out.insert(tth, m);
    }
    return out;
}

#endif
