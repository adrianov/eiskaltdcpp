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

#include "FileBrowserModel.h"
#include "filebrowser/ListFilter.h"

#include <QSortFilterProxyModel>
#include <QStringList>

namespace dcpp {
class DirectoryListing;
}

/** Left-pane directory tree filter (small row counts; sync QSortFilterProxyModel). */
class FileBrowserFilterProxy : public QSortFilterProxyModel {
Q_OBJECT
public:
    explicit FileBrowserFilterProxy(QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    void applyFilters(const QStringList &terms, qulonglong size, int sizeMode,
                      bool dirsOnly, bool filesOnly, const QStringList &exts,
                      const QString &pathPrefix);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    ListFilter filter_;
};
