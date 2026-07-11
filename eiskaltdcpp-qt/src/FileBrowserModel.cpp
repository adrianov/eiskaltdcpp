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

#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QFile>

#include "dcpp/UploadManager.h"

using namespace dcpp;

#include <set>

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

    if (restrictionsLoaded){
        QFile f(_q(Util::getPath(Util::PATH_USER_CONFIG) + "PerFolderLimit.conf"));

        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            QTextStream stream(&f);

            for (auto it = restrict_map.begin(); it != restrict_map.end(); ++it) {
                stream << it.value() << " " << it.key() << '\n';
            }

            stream.flush();

            f.close();

            dcpp::UploadManager::getInstance()->reloadRestrictions();
        }
    }
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

bool FileBrowserModel::canFetchMore(const QModelIndex &parent) const{
    if (!listing)
        return false;

    FileBrowserItem *item = parent.isValid()? static_cast<FileBrowserItem*>(parent.internalPointer()) : rootItem;

    return (item->dir && (item->dir->directories.size() != static_cast<size_t>(item->childCount())));
}

void FileBrowserModel::fetchBranch(const QModelIndex &parent, dcpp::DirectoryListing::Directory *dir){
    if (!dir)
        return;

    FileBrowserItem *root = parent.isValid()? static_cast<FileBrowserItem*>(parent.internalPointer()) : rootItem;
    FileBrowserItem *item;
    quint64 size = 0;
    QList<QVariant> data;

    size = dir->getTotalSize(true);

    data << _q(dir->getName())
         << WulforUtil::formatBytes(size)
         << size
         << "";

    item = new FileBrowserItem(data, root);
    item->dir = dir;

    beginInsertRows(parent, root->childCount(), root->childCount());
    {
        root->appendChild(item);
    }
    endInsertRows();
}

bool FileBrowserModel::hasChildren(const QModelIndex &parent) const{
    if (!parent.isValid())
        return true;

    FileBrowserItem *item = static_cast<FileBrowserItem*>(parent.internalPointer());

    return (item->dir && !item->dir->directories.empty());
}

void FileBrowserModel::fetchMore(const QModelIndex &parent){
    if (!listing)
        return;

    if (!parent.isValid())
        fetchBranch(parent, listing->getRoot());
    else{
        FileBrowserItem *item = static_cast<FileBrowserItem*>(parent.internalPointer());
        QModelIndex i = createIndexForItem(item);

        for (const auto &dir : item->dir->directories) //loading child directories
            fetchBranch(i, dir);

        sortFileBrowserItems(sortColumn, sortOrder, item);
    }
}

void FileBrowserModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (!rootItem || rootItem->childItems.empty() || column < 0 || column > columnCount()-1)
        return;

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

QString FileBrowserModel::createRemotePath(FileBrowserItem *item) const{
    QString s;
    FileBrowserItem * pitem;

    if (!item) {
        return s;
    }

    pitem = item;
    s = pitem->data(COLUMN_FILEBROWSER_NAME).toString();

    while ((pitem = pitem->parent())) {
        // check for root entry
        if (pitem->parent()) {
            s = pitem->data(COLUMN_FILEBROWSER_NAME).toString() + "\\" + s;
        }
    }

    return s;
}

FileBrowserItem *FileBrowserModel::createRootForPath(const QString &path, FileBrowserItem *pathRoot){
    if (path.isEmpty() || path.isNull())
        return nullptr;

    QString _path = path;
    _path.replace("\\", "/");

    QStringList list = _path.split("/", WULFOR_SKIP_EMPTY);
    FileBrowserItem *root = pathRoot?pathRoot:rootItem;

    if (list.empty() || !root)
        return nullptr;

    for (const auto &s : list){
        if (s.isEmpty())
            continue;

        if (!root)
            return nullptr;

        if (s == ".." && root->parent()){
            root = root->parent();

            continue;
        }
        else if (s == ".")
            continue;

        bool found = false;

        if (root->dir && !root->dir->directories.empty() && !root->childCount()) //Load child items
            fetchMore(createIndexForItem(root));

        for (const auto &item : root->childItems){
            if (!item->dir)
                continue;

            QString name = (item == rootItem ? "" : item->data(COLUMN_FILEBROWSER_NAME).toString());

            if (!name.compare(s, Qt::CaseSensitive)){
                root = item;
                found = true;

                break;
            }
        }

        if (!found)
            return root;
    }

    return root;
}

QModelIndex FileBrowserModel::createIndexForItem(FileBrowserItem *item){
    if (!rootItem || !item)
        return QModelIndex();

    return createIndex(item->row(), 0, item);
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

void FileBrowserModel::loadRestrictions(){
    QFile f(_q(Util::getPath(Util::PATH_USER_CONFIG) + "PerFolderLimit.conf"));

    if (f.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&f);
        QString line = "";

        while (!(line = stream.readLine(0)).isNull()){
            QStringList list = line.split(' ');

            if (list.size() < 2)
                continue;

            bool ok = false;
            unsigned size = 0;

            size = list.at(0).toUInt(&ok);

            if (!ok)
                continue;

            QString virt_path = line.remove(0, list.at(0).length() + 1);

            if (!virt_path.startsWith('/'))
                virt_path.prepend('/');

            if (!virt_path.isEmpty() && size > 0 && !restrict_map.contains(virt_path))
                restrict_map.insert(virt_path, size);
        }

        restrictionsLoaded = true;

        f.close();
    }
}

void FileBrowserModel::updateRestriction(QModelIndex &i, unsigned size){
    FileBrowserItem *item = static_cast<FileBrowserItem*>(i.internalPointer());
    QString path = "";//virtual path
    QStringList dirs = createRemotePath(item).split("\\");

    if (dirs.size() >= 2){
        dirs.removeFirst();

        path = "/" + dirs.join("/");
    }
    else
        path = "/";

    if (!size && restrict_map.contains(path))
        restrict_map.remove(path);
    else {
        restrict_map[path] = size;
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
    emit layoutChanged();
}
