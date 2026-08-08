/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "ShareIndex.h"
#include "sharebrowser/AsyncRunner.h"

#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QVariant>

void SearchFrame::queueMediaEnrich(const QStringList &tths)
{
#ifdef USE_QT_SQLITE
    Q_D(SearchFrame);
    if (tths.isEmpty())
        return;
    for (const QString &t : tths) {
        if (!t.isEmpty())
            d->pendingMediaTths.append(t);
    }
    if (d->mediaEnrichPending)
        return;
    d->mediaEnrichPending = true;
    QMetaObject::invokeMethod(this, "flushMediaEnrich", Qt::QueuedConnection);
#else
    Q_UNUSED(tths);
#endif
}

void SearchFrame::flushMediaEnrich()
{
#ifdef USE_QT_SQLITE
    Q_D(SearchFrame);
    d->mediaEnrichPending = false;
    // Do not gate on stop: rows already in the table should still get media.
    if (d->pendingMediaTths.isEmpty())
        return;
    if (d->mediaEnrichBusy) {
        d->mediaEnrichPending = true;
        return;
    }

    QStringList batch;
    QSet<QString> seen;
    for (const QString &t : d->pendingMediaTths) {
        if (t.isEmpty() || seen.contains(t))
            continue;
        seen.insert(t);
        batch << t;
    }
    d->pendingMediaTths.clear();
    if (batch.isEmpty())
        return;

    d->mediaEnrichBusy = true;
    QPointer<SearchFrame> guard(this);
    AsyncRunner *runner = new AsyncRunner(nullptr);
    runner->setRunFunction([guard, batch]() {
        ShareIndex *idx = ShareIndex::getInstance();
        const auto found = idx->mediaByTth(batch);
        idx->releaseThreadDb();

        QVariantMap packed;
        for (auto it = found.constBegin(); it != found.constEnd(); ++it) {
            if (it->isEmpty())
                continue;
            QVariantMap m;
            m.insert(QStringLiteral("bitrate"), it->bitrate);
            m.insert(QStringLiteral("resolution"), it->resolution);
            m.insert(QStringLiteral("video"), it->video);
            m.insert(QStringLiteral("audio"), it->audio);
            packed.insert(it.key(), m);
        }
        // Always deliver (even empty) so mediaEnrichBusy is cleared on the UI thread.
        if (guard) {
            QMetaObject::invokeMethod(guard.data(), "applyMediaEnrich", Qt::QueuedConnection,
                                      Q_ARG(QVariant, packed));
        }
    });
    connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
#endif
}

void SearchFrame::applyMediaEnrich(const QVariant &packed)
{
#ifdef USE_QT_SQLITE
    Q_D(SearchFrame);
    d->mediaEnrichBusy = false;

    const QVariantMap outer = packed.toMap();
    QHash<QString, QVariantMap> media;
    for (auto it = outer.constBegin(); it != outer.constEnd(); ++it)
        media.insert(it.key(), it.value().toMap());
    if (!media.isEmpty() && d->model)
        d->model->applyMediaByTth(media);

    if (!d->pendingMediaTths.isEmpty() && !d->mediaEnrichPending) {
        d->mediaEnrichPending = true;
        QMetaObject::invokeMethod(this, "flushMediaEnrich", Qt::QueuedConnection);
    }
#else
    Q_UNUSED(packed);
#endif
}
