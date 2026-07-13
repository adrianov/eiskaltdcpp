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
#include <QString>
#include <QPixmap>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QRegExp>

#include "dcpp/stdinc.h"
#include "dcpp/SearchResult.h"
#include "dcpp/SearchManager.h"

static const unsigned COLUMN_SF_COUNT          = 0;
static const unsigned COLUMN_SF_FILENAME       = 1;
static const unsigned COLUMN_SF_EXTENSION      = 2;
static const unsigned COLUMN_SF_SIZE           = 3;
static const unsigned COLUMN_SF_ESIZE          = 4;
static const unsigned COLUMN_SF_TTH            = 5;
static const unsigned COLUMN_SF_PATH           = 6;
static const unsigned COLUMN_SF_NICK           = 7;
static const unsigned COLUMN_SF_FREESLOTS      = 8;
static const unsigned COLUMN_SF_ALLSLOTS       = 9;
static const unsigned COLUMN_SF_IP             = 10;
static const unsigned COLUMN_SF_HUB            = 11;
static const unsigned COLUMN_SF_HOST           = 12;

class SearchListException{
    public:

    enum Type{
        Sort=0,
        Add,
        Unkn
    };

        SearchListException();
        SearchListException(const SearchListException&);
        SearchListException(const QString& message, Type type);
        virtual ~SearchListException();

        SearchListException &operator=(const SearchListException&);

        QString message;
        Type type;
};

class SearchItem
{

public:
    SearchItem(const QList<QVariant> &data, SearchItem *parent = nullptr);
    virtual ~SearchItem();

    void appendChild(SearchItem *child);

    SearchItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    SearchItem *parent() const;
    bool exists(const QString &user_cid) const;
    /** Cached share/finished path for this TTH; empty if not local. */
    QString localPath() const;
    /** Drop cached path so the next localPath() lookup runs again. */
    void clearLocalPath();
    /** True when this TTH is already in the download queue. */
    bool isQueued() const;
    /** Drop cached queue flag so the next isQueued() lookup runs again. */
    void clearQueued();

    unsigned count;

    QString cid;

    bool isDir;

    QList<SearchItem*> childItems;
private:

    QList<QVariant> itemData;
    SearchItem *parentItem;
    mutable bool localChecked = false;
    mutable QString localCached;
    mutable bool queuedChecked = false;
    mutable bool queuedCached = false;
};

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT
    typedef QVariantMap VarMap;
public:

    SearchModel(QObject *parent = nullptr);
    ~SearchModel();

    /** */
    QVariant data(const QModelIndex &, int) const;
    /** */
    Qt::ItemFlags flags(const QModelIndex &) const;
    /** */
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    /** */
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const;
    /** */
    QModelIndex parent(const QModelIndex &index) const;
    /** */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    /** */
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    /** */
    bool hasChildren(const QModelIndex &parent) const;
    /** sort list */
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    /** */
    QModelIndex createIndexForItem(SearchItem*);

    void setFilterRole(int);

    /** */
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

    /** */
    int getSortColumn() const;
    /** */
    void setSortColumn(int);
    /** */
    Qt::SortOrder getSortOrder() const;
    /** */
    void setSortOrder(Qt::SortOrder);

    /** Clear model and redraw view*/
    void clearModel();
    /** */
    void removeItem(const SearchItem*);

    /** Re-resolve local path and queue state for one TTH, or all after async moves. */
    void refreshLocal(const QString &tth);

    /** Run one Count-column root sort after a batch of grouped inserts. */
    void flushDeferredSort();

public Q_SLOTS:
    /** */
    bool addResultPtr(const VarMap&);

private:
    /** */
    bool okToFind(const SearchItem*);
    /** */
    int filterRole;
    /** */
    int sortColumn;
    /** */
    Qt::SortOrder sortOrder;
    /** True when grouped inserts need one Count-column root sort. */
    bool countSortPending = false;
    /** */
    SearchItem *rootItem;
    /** */
    QHash<QString, SearchItem*> tths;
    /** Directories grouped by path + name (same manner as TTH for files). */
    QHash<QString, SearchItem*> dirs;

    static QString dirGroupKey(const QString &path, const QString &file);
};
