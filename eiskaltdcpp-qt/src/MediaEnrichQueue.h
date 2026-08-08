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
#include <QStringList>
#include <QVariant>

/**
 * Coalesced background ShareIndex media lookup by TTH.
 * Emits ready(packed TTH→media map) on the host thread; always delivers so busy clears.
 */
class MediaEnrichQueue : public QObject {
    Q_OBJECT
public:
    explicit MediaEnrichQueue(QObject *parent = nullptr);

    void queue(const QStringList &tths);
    void clearPending();

Q_SIGNALS:
    void ready(const QVariant &packed);

private Q_SLOTS:
    void flush();
    void deliver(const QVariant &packed);

private:
    QStringList pending_;
    bool flushQueued_ = false;
    bool busy_ = false;
};
