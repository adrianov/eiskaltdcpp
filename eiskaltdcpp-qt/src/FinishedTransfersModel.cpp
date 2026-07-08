/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfersModel.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QFileInfo>
#include <QList>
#include <QStringList>
#include <QPalette>
#include <QColor>
#include <QDir>

#include "FinishedTransfersModel.h"
#include "FinishedTransfersModelSort.h"
#include "SearchFrame.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"
#include "dcpp/ShareManager.h"

#ifdef _DEBUG_QT_UI
#include <QtDebug>
#endif

using namespace dcpp;

namespace {

bool isDownloadFileList(const string& path) {
    if(path.empty())
        return false;

    const string listPath = Util::getListPath();
    if(!listPath.empty() && path.size() >= listPath.size() &&
       Util::stricmp(path.substr(0, listPath.size()).c_str(), listPath.c_str()) == 0)
        return true;

    if(path.size() >= 4 && Util::stricmp(path.substr(path.size() - 4).c_str(), ".xml") == 0)
        return true;
    if(path.size() >= 7 && Util::stricmp(path.substr(path.size() - 7).c_str(), ".xml.bz2") == 0)
        return true;

    return Util::stricmp(Util::getFileExt(path).c_str(), ".DcLst") == 0;
}

} // namespace

FinishedTransfersModel::FinishedTransfersModel(QObject *parent):
        QAbstractItemModel(parent), sortColumn(COLUMN_FINISHED_TIME), sortOrder(Qt::AscendingOrder),
        hideFileLists(false), requireFullFile(false), bulkLoadDepth(0)
{
    QList<QVariant> userData;
    userData << tr("User")<< tr("Files") << tr("Time") << tr("Transferred")
             << tr("Speed") << tr("Elapsed time") << tr("Full");

    userItem = new FinishedTransfersItem(userData);

    QList<QVariant> fileData;
    fileData << tr("Filename") << tr("Path") << tr("Time") << tr("User")
             << tr("Transferred") << tr("Speed")   << tr("Check sum")
             << tr("Target") << tr("Elapsed time") << tr("Full");

    fileItem = new FinishedTransfersItem(fileData);

    rootItem = fileItem;

    file_header_table.insert(COLUMN_FINISHED_NAME,      "FNAME");
    file_header_table.insert(COLUMN_FINISHED_PATH,      "PATH");
    file_header_table.insert(COLUMN_FINISHED_TIME,      "TIME");
    file_header_table.insert(COLUMN_FINISHED_USER,      "USERS");
    file_header_table.insert(COLUMN_FINISHED_TR,        "TR");
    file_header_table.insert(COLUMN_FINISHED_SPEED,     "SPEED");
    file_header_table.insert(COLUMN_FINISHED_CRC32,     "CRC32");
    file_header_table.insert(COLUMN_FINISHED_TARGET,    "TARGET");
    file_header_table.insert(COLUMN_FINISHED_ELAPS,     "ELAP");
    file_header_table.insert(COLUMN_FINISHED_FULL,      "FULL");

    user_header_table.insert(COLUMN_FINISHED_NAME,      "NICK");
    user_header_table.insert(COLUMN_FINISHED_PATH,      "FILES");
    user_header_table.insert(COLUMN_FINISHED_TIME,      "TIME");
    user_header_table.insert(COLUMN_FINISHED_USER,      "TR");
    user_header_table.insert(COLUMN_FINISHED_TR,        "SPEED");
    user_header_table.insert(COLUMN_FINISHED_SPEED,     "ELAP");
    user_header_table.insert(COLUMN_FINISHED_CRC32,     "FULL");
}

FinishedTransfersModel::~FinishedTransfersModel()
{
    delete userItem;
    delete fileItem;
}

int FinishedTransfersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FinishedTransfersItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant FinishedTransfersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FinishedTransfersItem *item = static_cast<FinishedTransfersItem*>(index.internalPointer());

    switch(role) {
        case Qt::DecorationRole: // icon
        {
            if (rootItem == fileItem){
                if (index.column() == COLUMN_FINISHED_NAME)
                    return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_FINISHED_TARGET).toString()), 16);
            }

            break;
        }
        case Qt::DisplayRole:
        {
            if (rootItem == fileItem){
                if (index.column() == COLUMN_FINISHED_ELAPS)
                    return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_ELAPS).toLongLong()/1000L));
                else if (index.column() == COLUMN_FINISHED_SPEED)
                    return tr("%1/s").arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_SPEED).toLongLong()));
                else if (index.column() == COLUMN_FINISHED_TR)
                    return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong());
                else if (index.column() == COLUMN_FINISHED_FULL)
                    return (item->data(COLUMN_FINISHED_FULL).toBool()? "1" : "0");
            }
            else {
                if (index.column() == COLUMN_FINISHED_SPEED)
                    return _q(Util::formatSeconds(item->data(COLUMN_FINISHED_SPEED).toLongLong()/1000L));
                else if (index.column() == COLUMN_FINISHED_TR)
                    return tr("%1/s").arg(WulforUtil::formatBytes(item->data(COLUMN_FINISHED_TR).toLongLong()));
                else if (index.column() == COLUMN_FINISHED_USER)
                    return WulforUtil::formatBytes(item->data(COLUMN_FINISHED_USER).toLongLong());
                else if (index.column() == COLUMN_FINISHED_CRC32)
                    return (item->data(COLUMN_FINISHED_CRC32).toBool()? "1" : "0");
            }

            return item->data(index.column());
        }
        case Qt::TextAlignmentRole:
        {
            break;
        }
        case Qt::ForegroundRole:
        {
            break;
        }
        case Qt::BackgroundColorRole:
            break;
        case Qt::ToolTipRole:
            break;
    }

    return QVariant();
}

Qt::ItemFlags FinishedTransfersModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FinishedTransfersModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex FinishedTransfersModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FinishedTransfersItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FinishedTransfersItem*>(parent.internalPointer());

    FinishedTransfersItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex FinishedTransfersModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int FinishedTransfersModel::rowCount(const QModelIndex &parent) const
{
    FinishedTransfersItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<FinishedTransfersItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void FinishedTransfersModel::sort(int column, Qt::SortOrder order) {
    emit layoutAboutToBeChanged();

    sortColumn = column;
    sortOrder = order;

    if (rootItem == fileItem)
        FinishedTransfersModelSort::sortFiles(column, order, rootItem->childItems);
    else
        FinishedTransfersModelSort::sortUsers(column, order, rootItem->childItems);

    emit layoutChanged();
}

void FinishedTransfersModel::clearModel(){
    beginResetModel();
    {
        qDeleteAll(userItem->childItems);
        qDeleteAll(fileItem->childItems);

        userItem->childItems.clear();
        fileItem->childItems.clear();

        file_hash.clear();
        user_hash.clear();
    }
    endResetModel();
}

bool FinishedTransfersModel::acceptDownloadFile(const QVariantMap &params) const {
    if (!hideFileLists && !requireFullFile)
        return true;

    const string target = _tq(params["TARGET"].toString());
    const string path = _tq(params["PATH"].toString() + params["FNAME"].toString());

    if (hideFileLists && (isDownloadFileList(target) || isDownloadFileList(path)))
        return false;

    if (requireFullFile && !params["FULL"].toBool())
        return false;

    return true;
}

bool FinishedTransferProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        return false;

    if (!hideFileLists_ && !requireFullFile_)
        return true;

    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    const QModelIndex targetIndex = model->index(sourceRow, COLUMN_FINISHED_TARGET, sourceParent);
    const QModelIndex pathIndex = model->index(sourceRow, COLUMN_FINISHED_PATH, sourceParent);
    const QModelIndex nameIndex = model->index(sourceRow, COLUMN_FINISHED_NAME, sourceParent);
    const QModelIndex fullIndex = model->index(sourceRow, COLUMN_FINISHED_FULL, sourceParent);

    const string target = _tq(model->data(targetIndex, Qt::DisplayRole).toString());
    const string path = _tq(model->data(pathIndex, Qt::DisplayRole).toString() +
                            model->data(nameIndex, Qt::DisplayRole).toString());

    if (hideFileLists_ && (isDownloadFileList(target) || isDownloadFileList(path)))
        return false;

    if (requireFullFile_ && model->data(fullIndex, Qt::DisplayRole).toString() != QLatin1String("1"))
        return false;

    return true;
}

void FinishedTransfersModel::addFile(const QVariantMap &params){
    if (!acceptDownloadFile(params))
        return;

    FinishedTransfersItem *item = findFile(params["TARGET"].toString());

    if (!item)
        return;

    for (int i = 0; i < fileItem->columnCount(); i++){
        if (file_header_table[i] == "USERS"){
            QStringList users = params[file_header_table[i]].toString().split(" ");
            QStringList old_users = item->data(i).toString().split(" ");

            if (users.isEmpty())
                continue;
            else{
                for (const auto nick : users){
                    if (!old_users.contains(nick))
                        old_users.push_back(nick);
                }

                item->updateColumn(i, old_users.join(" "));
            }
        }
        else
            item->updateColumn(i, params[file_header_table[i]]);
    }

    if (bulkLoadDepth == 0 && rootItem == fileItem)
        sort();
    else
        emit dataChanged(createIndex(item->row(), COLUMN_FINISHED_NAME, item), createIndex(item->row(), COLUMN_FINISHED_FULL, item));
}

void FinishedTransfersModel::addUser(const QVariantMap &params){
    FinishedTransfersItem *item = findUser(params["CID"].toString());

    if (!item)
        return;

    for (int i = 0; i < userItem->columnCount(); i++){
        if (user_header_table[i] == "NICK"){
            QString user = params[user_header_table[i]].toString();

            if (user.trimmed().isEmpty() || user.trimmed().isNull())
                continue;
            else
                item->updateColumn(i, user);
        }
        else
            item->updateColumn(i, params[user_header_table[i]]);
    }

    if (bulkLoadDepth == 0 && rootItem == userItem)
        sort();
    else
        emit dataChanged(createIndex(item->row(), COLUMN_FINISHED_NAME, item), createIndex(item->row(), COLUMN_FINISHED_CRC32, item));
}

void FinishedTransfersModel::remFile(const QString &file){
    FinishedTransfersItem *item = findFile(file);

    if (!item)
        return;

    file_hash.remove(file);

    beginRemoveRows(QModelIndex(), item->row(), item->row());
    {
        fileItem->childItems.removeAt(item->row());

        delete item;
    }
    endRemoveRows();
}

void FinishedTransfersModel::remUser(const QString &cid){
    FinishedTransfersItem *item = findUser(cid);

    if (!item)
        return;

    user_hash.remove(cid);

    beginRemoveRows(QModelIndex(), item->row(), item->row());
    {
        userItem->childItems.removeAt(item->row());

        delete item;
    }
    endRemoveRows();
}

void FinishedTransfersModel::switchViewType(FinishedTransfersModel::ViewType t){
    beginResetModel();
    switch (t){
        case FileView:
            rootItem = fileItem;
            break;
        case UserView:
            rootItem = userItem;
            break;
    }
    endResetModel();
    sort();
}

FinishedTransfersItem *FinishedTransfersModel::findFile(const QString &fname){
    if (fname.isEmpty())
        return nullptr;

    auto it = file_hash.find(fname);

    if (it != file_hash.constEnd())
        return const_cast<FinishedTransfersItem*>(it.value());

    FinishedTransfersItem *item = new FinishedTransfersItem(QList<QVariant>() << "" << "" << "" << ""
                                                                              << "" << "" << "" << ""
                                                                              << "" << false,
                                                            fileItem);
    if (fileItem == rootItem){
        emit beginInsertRows(QModelIndex(), rootItem->childCount(), rootItem->childCount());
        {
            fileItem->appendChild(item);
        }
        emit endInsertRows();
    }
    else
        fileItem->appendChild(item);

    file_hash.insert(fname, item);

    return item;
}

FinishedTransfersItem *FinishedTransfersModel::findUser(const QString &cid){
    if (cid.isEmpty())
        return nullptr;

    auto it = user_hash.find(cid);

    if (it != user_hash.constEnd())
        return const_cast<FinishedTransfersItem*>(it.value());

    FinishedTransfersItem *item = new FinishedTransfersItem(QList<QVariant>() << "" << "" << ""
                                                                              << "" << "" << ""
                                                                              << false,
                                                            userItem);
    if (userItem == rootItem){
        emit beginInsertRows(QModelIndex(), rootItem->childCount(), rootItem->childCount());
        {
            userItem->appendChild(item);
        }
        emit endInsertRows();
    }
    else
        userItem->appendChild(item);;

    user_hash.insert(cid, item);

    return item;
}

void FinishedTransfersModel::repaint(){
    emit layoutChanged();
}

FinishedTransfersItem::FinishedTransfersItem(const QList<QVariant> &data, FinishedTransfersItem *parent) :
    itemData(data),
    parentItem(parent)
{
}

FinishedTransfersItem::~FinishedTransfersItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void FinishedTransfersItem::appendChild(FinishedTransfersItem *item) {
    childItems.append(item);
}

FinishedTransfersItem *FinishedTransfersItem::child(int row) {
    return ((row >= 0 && row <= childItems.count()-1)? childItems.value(row) : nullptr);
}

int FinishedTransfersItem::childCount() const {
    return childItems.count();
}

int FinishedTransfersItem::columnCount() const {
    return itemData.count();
}

QVariant FinishedTransfersItem::data(int column) const {
    return itemData.value(column);
}

FinishedTransfersItem *FinishedTransfersItem::parent() const{
    return parentItem;
}

int FinishedTransfersItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FinishedTransfersItem*>(this));

    return 0;
}

void FinishedTransfersItem::updateColumn(int column, QVariant var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}
