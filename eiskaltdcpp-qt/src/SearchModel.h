/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>

#include "search/SearchItem.h"

class SearchModel : public QAbstractItemModel {
    Q_OBJECT
    typedef QVariantMap VarMap;
public:
    SearchModel(QObject *parent = nullptr);
    ~SearchModel();

    QVariant data(const QModelIndex &, int) const;
    Qt::ItemFlags flags(const QModelIndex &) const;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QModelIndex createIndexForItem(SearchItem*);
    void setFilterRole(int);

    bool addResult(
            const QString &file,
            qulonglong size,
            const QString &tth,
            const QString &path,
            const QString &nick,
            const int free_slots,
            const int all_slots,
            const QString &ip,
            const QString &hub,
            const QString &host,
            const QString &cid,
            const bool isDir);

    int getSortColumn() const;
    void setSortColumn(int);
    Qt::SortOrder getSortOrder() const;
    void setSortOrder(Qt::SortOrder);

    void clearModel();
    void removeItem(const SearchItem*);

    /** Re-resolve local path and queue state for one TTH, or all after async moves. */
    void refreshLocal(const QString &tth);

    /** Run one Count-column root sort after a batch of grouped inserts. */
    void flushDeferredSort();

    /** Fill empty media cells for grouped TTH roots (and children). */
    void applyMediaByTth(const QHash<QString, QVariantMap> &media);
    /** True when the TTH root already has any media field. */
    bool hasMedia(const QString &tth) const;

public Q_SLOTS:
    bool addResultPtr(const VarMap&);

private:
    bool okToFind(const SearchItem*);
    int filterRole;
    int sortColumn;
    Qt::SortOrder sortOrder;
    /** True when grouped inserts need one Count-column root sort. */
    bool countSortPending = false;
    SearchItem *rootItem;
    QHash<QString, SearchItem*> tths;
    /** Directories grouped by path + name (same manner as TTH for files). */
    QHash<QString, SearchItem*> dirs;

    static QString dirGroupKey(const QString &path, const QString &file);
};
