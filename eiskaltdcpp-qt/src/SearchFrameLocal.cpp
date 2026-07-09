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

    AsyncRunner *runner = new AsyncRunner(frame);
    runner->setRunFunction([frame, filter]() {
        ShareIndex *idx = ShareIndex::getInstance();
        const QList<QVariantMap> rows = idx->search(filter);
        for (const QVariantMap &map : rows) {
            QMetaObject::invokeMethod(frame, "addResult", Qt::QueuedConnection,
                                      Q_ARG(QVariantMap, map));
        }
        idx->releaseThreadDb();
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
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

    AsyncRunner *runner = new AsyncRunner(frame);
    runner->setRunFunction([frame]() {
        ShareIndex *idx = ShareIndex::getInstance();
        const QString text = statsText(idx->indexStats());
        QMetaObject::invokeMethod(frame, "setIndexStats", Qt::QueuedConnection,
                                  Q_ARG(QString, text));
        idx->releaseThreadDb();
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
#else
    Q_UNUSED(frame);
#endif
}

} // namespace SearchFrameLocal
