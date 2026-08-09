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
#include <QVariant>

static const unsigned COLUMN_SF_COUNT          = 0;
static const unsigned COLUMN_SF_FILENAME       = 1;
static const unsigned COLUMN_SF_EXTENSION      = 2;
static const unsigned COLUMN_SF_SIZE           = 3;
static const unsigned COLUMN_SF_PATH           = 4;
static const unsigned COLUMN_SF_ESIZE          = 5;
static const unsigned COLUMN_SF_TTH            = 6;
static const unsigned COLUMN_SF_NICK           = 7;
static const unsigned COLUMN_SF_FREESLOTS      = 8;
static const unsigned COLUMN_SF_ALLSLOTS       = 9;
static const unsigned COLUMN_SF_IP             = 10;
static const unsigned COLUMN_SF_HUB            = 11;
static const unsigned COLUMN_SF_HOST           = 12;
/** Media from ShareIndex (appended so saved header state stays stable). */
static const unsigned COLUMN_SF_BR             = 13;
static const unsigned COLUMN_SF_WH             = 14;
static const unsigned COLUMN_SF_MVIDEO         = 15;
static const unsigned COLUMN_SF_MAUDIO         = 16;
static const unsigned COLUMN_SF_LAST           = COLUMN_SF_MAUDIO;

class SearchListException {
public:
    enum Type {
        Sort = 0,
        Add,
        Unkn
    };

    SearchListException();
    SearchListException(const SearchListException&);
    SearchListException(const QString &message, Type type);
    virtual ~SearchListException();

    SearchListException &operator=(const SearchListException&);

    QString message;
    Type type;
};

/** One search-result row (TTH/dir group root or a source child). */
class SearchItem {
public:
    SearchItem(const QList<QVariant> &data, SearchItem *parent = nullptr);
    virtual ~SearchItem();

    void appendChild(SearchItem *child);
    void insertChild(int row, SearchItem *child);
    void removeChild(int row);
    void clearChildren();
    /** Reorder only; unique-source count unchanged. */
    void sortChildren(int column, Qt::SortOrder order);
    /** Insertion index in an already-sorted child list (does not insert). */
    int sortedInsertRow(int column, Qt::SortOrder order, SearchItem *item) const;

    SearchItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    void updateColumn(int column, const QVariant &value);
    int row() const;
    SearchItem *parent() const;
    /** Self or child with this CID; nullptr if absent. */
    SearchItem *findSource(const QString &user_cid);
    const QList<SearchItem*> &children() const { return childItems; }
    /** Cached share/finished path for this TTH; empty if not local. */
    QString localPath() const;
    /** Drop cached path so the next localPath() lookup runs again. */
    void clearLocalPath();
    /** True when this TTH is already in the download queue. */
    bool isQueued() const;
    /** Drop cached queue flag so the next isQueued() lookup runs again. */
    void clearQueued();
    /** Gray wash: all sources offline. */
    bool mutedTint() const { return mutedTint_; }
    void setMutedTint(bool on) { mutedTint_ = on; }

    QString cid;
    bool isDir;

private:
    QList<SearchItem*> childItems;
    QList<QVariant> itemData;
    SearchItem *parentItem;
    mutable bool localChecked = false;
    mutable QString localCached;
    mutable bool queuedChecked = false;
    mutable bool queuedCached = false;
    /** Unique source count; -1 means dirty. */
    mutable int countCached = -1;
    bool mutedTint_ = false;
};
