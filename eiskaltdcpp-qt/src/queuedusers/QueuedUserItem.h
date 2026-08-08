/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QList>
#include <QString>
#include <QVariant>

/** One node in the waiting-upload queue tree (user or requested file). */
class QueuedUserItem
{
public:
    QueuedUserItem(const QList<QVariant> &data, QueuedUserItem *parent = nullptr);
    ~QueuedUserItem();

    void appendChild(QueuedUserItem *child);
    QueuedUserItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    QueuedUserItem *parent() const;

    QString cid;
    QString file;
    QString hub;
    QList<QueuedUserItem*> childItems;

private:
    QList<QVariant> itemData;
    QueuedUserItem *parentItem;
};
