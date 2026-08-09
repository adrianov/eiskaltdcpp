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

#include "filebrowser/ListFilter.h"

#include <QAbstractProxyModel>
#include <QHash>
#include <QStringList>
#include <QVector>

/**
 * Flat list proxy: identity when unfiltered, compact source-row map when filtered.
 * Avoids QSortFilterProxyModel's O(n) mapping rebuild on every keystroke.
 */
class ListFilterProxy : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit ListFilterProxy(QObject *parent = nullptr)
        : QAbstractProxyModel(parent) {}
    ~ListFilterProxy() override { filter_.join(this); }

    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void sort(int column, Qt::SortOrder order) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

    void applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                      bool dirsOnly, bool filesOnly, const QStringList &exts,
                      const QString &pathPrefix);

private:
    void clearMap();
    void setIdentity();
    void setRows(QVector<int> rows);
    void scheduleFilter();

    ListFilter filter_;
    QString pathPrefix_;
    QVector<int> rows_;
    QHash<int, int> sourceToProxy_;
    bool filtered_ = false;
};
