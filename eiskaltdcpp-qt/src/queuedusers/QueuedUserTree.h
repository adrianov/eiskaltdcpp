/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "queuedusers/QueuedUserItem.h"

#include <QHash>
#include <QVariantMap>
#include <Qt>

/**
 * Waiting-upload queue tree: users keyed by CID, each with requested-file children.
 * Owns storage; Qt model notifications stay in QueuedUsersModel.
 */
class QueuedUserTree
{
public:
    typedef QVariantMap VarMap;

    explicit QueuedUserTree(const QList<QVariant> &headers);
    ~QueuedUserTree();

    QueuedUserItem *root() const { return rootItem; }
    QueuedUserItem *user(const QString &cid) const;

    QueuedUserItem *addUser(const VarMap &map);
    QueuedUserItem *addFile(QueuedUserItem *user, const VarMap &map);
    /** Detach user subtree from root; caller deletes. */
    QueuedUserItem *takeUser(const QString &cid);

    void sort(int column = -1, Qt::SortOrder order = Qt::AscendingOrder);

private:
    static QueuedUserItem *makeItem(const QList<QVariant> &data, QueuedUserItem *parent,
                                    const QString &cid, const QString &file, const QString &hub);
    void sortItems(QList<QueuedUserItem*> &items) const;

    QueuedUserItem *rootItem;
    QHash<QString, QueuedUserItem*> cids;
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
};
