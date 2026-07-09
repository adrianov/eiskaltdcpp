/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrameLocal.h"
#include "SearchFrame.h"
#include "ShareIndex.h"
#include "ShareBrowser.h"
#include "WulforUtil.h"

#include <QMetaObject>
#include <QPointer>
#include <QVariant>

#include <functional>

namespace SearchFrameLocal {

namespace {

QString compactCount(qint64 n)
{
    if (n >= 1000000000LL)
        return QString::number(n / 1000000000.0, 'f', 1) + QLatin1String(" B");
    if (n >= 1000000LL)
        return QString::number(n / 1000000.0, 'f', 1) + QLatin1String(" M");
    if (n >= 1000LL)
        return QString::number(n / 1000.0, 'f', 1) + QLatin1String(" K");
    return QString::number(n);
}

QString statsText(const ShareIndex::IndexStats &stats)
{
    if (stats.files <= 0 && stats.dbBytes <= 0)
        return QString();

    return QObject::tr("%1 files indexed\nDB size: %2")
            .arg(compactCount(stats.files), WulforUtil::formatBytes(stats.dbBytes));
}

void startDetached(std::function<void()> work)
{
    // Do not parent to SearchFrame: closing the frame must not destroy a live QThread.
    AsyncRunner *runner = new AsyncRunner(nullptr);
    runner->setRunFunction(std::move(work));
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

} // namespace

void startLocalSearch(SearchFrame *frame, const QStringList &terms, bool isHash,
                      bool dirsOnly, bool filesOnly, qint64 size, int sizeMode,
                      const QStringList &extensions)
{
    if (!frame || terms.isEmpty())
        return;

    ShareIndex::SearchFilter filter;
    filter.terms = terms;
    filter.extensions = extensions;
    filter.isHash = isHash;
    filter.dirsOnly = dirsOnly;
    filter.filesOnly = filesOnly;
    filter.size = size;
    filter.sizeMode = sizeMode;
    filter.limit = 500;

    QPointer<SearchFrame> guard(frame);
    startDetached([guard, filter]() {
        ShareIndex *idx = ShareIndex::getInstance();
        const QList<QVariantMap> rows = idx->search(filter);
        idx->releaseThreadDb();
        if (!guard || rows.isEmpty())
            return;
        // One queued slot instead of up to 500 layoutChanged storms on the GUI thread.
        QMetaObject::invokeMethod(guard.data(), "addResultsPacked", Qt::QueuedConnection,
                                  Q_ARG(QVariant, QVariant::fromValue(rows)));
    });
}

void upsertHubResult(const QVariantMap &map)
{
    ShareIndex::getInstance()->upsertFromSearch(map);
}

void refreshIndexStats(SearchFrame *frame)
{
#ifdef USE_QT_SQLITE
    if (!frame)
        return;

    QPointer<SearchFrame> guard(frame);
    startDetached([guard]() {
        ShareIndex *idx = ShareIndex::getInstance();
        const QString text = statsText(idx->indexStats());
        idx->releaseThreadDb();
        if (!guard)
            return;
        QMetaObject::invokeMethod(guard.data(), "setIndexStats", Qt::QueuedConnection,
                                  Q_ARG(QString, text));
    });
#else
    Q_UNUSED(frame);
#endif
}

} // namespace SearchFrameLocal
