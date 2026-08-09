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

#include <QList>
#include <QString>
#include <QtGlobal>

class TransferViewItem;

/**
 * File-group parent row in Transfer View: aggregates children into progress,
 * session speed/ETA, and identity columns (hubs, tags, nick).
 */
class TransferGroup
{
public:
    explicit TransferGroup(TransferViewItem *parent);

    bool valid() const;
    /** Recompute parent columns from current children + TransferSession. */
    void refresh();

private:
    struct Scan {
        int active = 0;
        qint64 bestQueuePos = 0;
        qint64 totalSize = 0;
        qlonglong progressPos = 0;
        QList<QString> hubs;
        QList<QString> tags;
        QString nick;
    };

    TransferViewItem *parent_;

    void scanChild(TransferViewItem *child, Scan &s) const;
    void noteActive(TransferViewItem *child, Scan &s) const;
    void finishProgress(Scan &s) const;
    qlonglong downloadProgress(const Scan &s) const;
    void writeSpeed(const Scan &s) const;
    QString progressStat(const Scan &s) const;
    void writeIdentity(const Scan &s) const;
    static QString joinList(const QList<QString> &parts);
};
