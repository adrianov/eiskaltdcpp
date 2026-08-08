/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MediaEnrichQueue.h"
#include "ShareIndex.h"
#include "sharebrowser/AsyncRunner.h"

#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QVariantMap>

MediaEnrichQueue::MediaEnrichQueue(QObject *parent)
    : QObject(parent)
{
}

void MediaEnrichQueue::queue(const QStringList &tths)
{
#ifdef USE_QT_SQLITE
    if (tths.isEmpty())
        return;
    for (const QString &t : tths) {
        if (!t.isEmpty())
            pending_.append(t);
    }
    if (flushQueued_)
        return;
    flushQueued_ = true;
    QMetaObject::invokeMethod(this, "flush", Qt::QueuedConnection);
#else
    Q_UNUSED(tths);
#endif
}

void MediaEnrichQueue::clearPending()
{
    pending_.clear();
    flushQueued_ = false;
}

void MediaEnrichQueue::flush()
{
#ifdef USE_QT_SQLITE
    flushQueued_ = false;
    if (pending_.isEmpty())
        return;
    if (busy_) {
        flushQueued_ = true;
        return;
    }

    QStringList batch;
    QSet<QString> seen;
    for (const QString &t : pending_) {
        if (t.isEmpty() || seen.contains(t))
            continue;
        seen.insert(t);
        batch << t;
    }
    pending_.clear();
    if (batch.isEmpty())
        return;

    busy_ = true;
    QPointer<MediaEnrichQueue> guard(this);
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
        if (guard) {
            QMetaObject::invokeMethod(guard.data(), "deliver", Qt::QueuedConnection,
                                      Q_ARG(QVariant, packed));
        }
    });
    connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
#endif
}

void MediaEnrichQueue::deliver(const QVariant &packed)
{
#ifdef USE_QT_SQLITE
    busy_ = false;
    emit ready(packed);
    if (!pending_.isEmpty() && !flushQueued_) {
        flushQueued_ = true;
        QMetaObject::invokeMethod(this, "flush", Qt::QueuedConnection);
    }
#else
    Q_UNUSED(packed);
#endif
}
