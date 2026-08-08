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

#include "queuedusers/QueuedUserTree.h"

#include <QAbstractItemModel>

/** Qt item model over QueuedUserTree (waiting upload queue). */
class QueuedUsersModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    typedef QueuedUserTree::VarMap VarMap;

    explicit QueuedUsersModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &, int) const override;
    Qt::ItemFlags flags(const QModelIndex &) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    void sort(int column = -1, Qt::SortOrder order = Qt::AscendingOrder) override;

    void addResult(const VarMap &map);
    void remResult(const VarMap &map);

private:
    QueuedUserItem *itemAt(const QModelIndex &index) const;
    QueuedUserTree tree;
};
