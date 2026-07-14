/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserModel.h"
#include "FileBrowserModelSort.h"
#include "WulforUtil.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QList>
#include <QStringList>

using namespace dcpp;

FileBrowserModel::FileBrowserModel(QObject *parent)
    : QAbstractItemModel(parent), listing(nullptr), restrictionsLoaded(false), ownList(false)
{
    QList<QVariant> rootItemCulumns;
    for (int k = 0; k < NUM_OF_COLUMNS; ++k)
        rootItemCulumns << QString();

    rootItem = new FileBrowserItem(rootItemCulumns, nullptr);

    sortColumn = COLUMN_FILEBROWSER_NAME;
    sortOrder = Qt::AscendingOrder;
}

FileBrowserModel::~FileBrowserModel()
{
    if (rootItem)
        delete rootItem;

    saveRestrictions();
}

int FileBrowserModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FileBrowserItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QModelIndex FileBrowserModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FileBrowserItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FileBrowserItem*>(parent.internalPointer());

    FileBrowserItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex FileBrowserModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    FileBrowserItem *childItem = static_cast<FileBrowserItem*>(index.internalPointer());
    FileBrowserItem *parentItem = childItem->parent();

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int FileBrowserModel::rowCount(const QModelIndex &parent) const
{
    FileBrowserItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FileBrowserItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void FileBrowserModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (!rootItem || rootItem->childItems.empty() || column < 0 || column > columnCount()-1)
        return;

    // Plain layout signals only — remapping persistent indexes here breaks
    // QSortFilterProxyModel (ShareBrowser left pane) and crashes in scrollTo.
    emit layoutAboutToBeChanged();
    sortFileBrowserItems(column, order, rootItem);
    emit layoutChanged();
}

FileBrowserItem *FileBrowserModel::getRootElem() const {
    return rootItem;
}

int FileBrowserModel::getSortColumn() const {
    return sortColumn;
}

void FileBrowserModel::setSortColumn(int c) {
    sortColumn = c;
}

Qt::SortOrder FileBrowserModel::getSortOrder() const {
    return sortOrder;
}

void FileBrowserModel::setSortOrder(Qt::SortOrder o) {
    sortOrder = o;
}

void FileBrowserModel::highlightDuplicates(){
    if (!rootItem || !rootItem->childCount())
        return;

    for (const auto &i : rootItem->childItems){
        const QString &tth = i->data(COLUMN_FILEBROWSER_TTH).toString();

        if (tth.isEmpty())
            continue;

        auto it = hash.find(tth);

        if (it != hash.end()){
            if (i->file != it.value())//Found duplicate
                i->isDuplicate = true;
        }
        else if (!i->file->getAdls()){
            hash.insert(tth, i->file);
        }
    }
}

void FileBrowserModel::clear(){
    beginRemoveRows(QModelIndex(), 0, (rowCount() >= 1? rowCount() : 1)-1);
    {
        qDeleteAll(rootItem->childItems);
        rootItem->childItems.clear();
    }
    endRemoveRows();
}

void FileBrowserModel::repaint(){
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}
