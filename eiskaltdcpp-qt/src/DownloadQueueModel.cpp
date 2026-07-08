/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "DownloadQueueModel.h"
#include "DownloadQueueModelSort.h"
#include "WulforUtil.h"
#include "AppTheme.h"

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
#include <QIcon>
#include <QPixmap>
#include <QFontMetrics>
#include <QStyleOptionProgressBar>
#include <QSize>

#include <dcpp/stdinc.h>
#include <dcpp/QueueManager.h>

#if _DEBUG_QT_UI
#include <QtDebug>
#endif

#if _DEBUG_QT_UI
static inline void printRoot(DownloadQueueItem *i, const QString &dlmtr){
    if (!i)
        return;

	qDebug() << dlmtr.toUtf8().constData() << i->data(COLUMN_DOWNLOADQUEUE_NAME).toString().toUtf8().constData();

    for (const auto &child : i->childItems)
        printRoot(child, dlmtr + "-");
}
#endif

class DownloadQueueModelPrivate{
public:
    /** */
    quint64 total_files;
    /** */
    quint64 total_size;
    /** */
    int sortColumn;
    /** */
    Qt::SortOrder sortOrder;
    /** */
    DownloadQueueItem *rootItem;
};

DownloadQueueModel::DownloadQueueModel(QObject *parent)
    : QAbstractItemModel(parent), d_ptr(new DownloadQueueModelPrivate())
{
    Q_D(DownloadQueueModel);

    d->total_files = 0;
    d->total_size  = 0;

    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Status") << tr("Size") << tr("Downloaded")
             << tr("Priority") << tr("User") << tr("Path") << tr("Exact size")
             << tr("Errors") << tr("Added") << QString("TTH");

    d->rootItem = new DownloadQueueItem(rootData, nullptr);

    d->sortColumn = COLUMN_DOWNLOADQUEUE_NAME;
    d->sortOrder = Qt::DescendingOrder;
}

DownloadQueueModel::~DownloadQueueModel()
{
    Q_D(DownloadQueueModel);

    if (d->rootItem)
        delete d->rootItem;

    delete d;
}

int DownloadQueueModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);

    if (parent.isValid())
        return static_cast<DownloadQueueItem*>(parent.internalPointer())->columnCount();
    else
        return d->rootItem->columnCount();
}

QVariant DownloadQueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    DownloadQueueItem *item = static_cast<DownloadQueueItem*>(index.internalPointer());

    switch(role) {
        case Qt::DecorationRole:
        {
            if (item->dir && index.column() == COLUMN_DOWNLOADQUEUE_NAME)
                return WICON_SIZE(WulforUtil::eiFOLDER_BLUE, 16);
            else if (index.column() == COLUMN_DOWNLOADQUEUE_NAME)
                return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_DOWNLOADQUEUE_NAME).toString()), 16);
        }
        case Qt::DisplayRole:
        {
            if ((index.column() == COLUMN_DOWNLOADQUEUE_DOWN || index.column() == COLUMN_DOWNLOADQUEUE_SIZE) && !item->dir)
                return WulforUtil::formatBytes(item->data(index.column()).toLongLong());
            else if ((index.column() == COLUMN_DOWNLOADQUEUE_DOWN || index.column() == COLUMN_DOWNLOADQUEUE_SIZE) && item->dir)
                break;
            else if (index.column() == COLUMN_DOWNLOADQUEUE_PRIO && !item->dir){
                QueueItem::Priority prio = static_cast<QueueItem::Priority>(item->data(COLUMN_DOWNLOADQUEUE_PRIO).toInt());

                QString prio_str = "";

                switch (prio){
                    case QueueItem::PAUSED:
                        prio_str = tr("Paused");
                        break;
                    case QueueItem::LOWEST:
                        prio_str = tr("Lowest");
                        break;
                    case QueueItem::LOW:
                        prio_str = tr("Low");
                        break;
                    case QueueItem::HIGH:
                        prio_str = tr("High");
                        break;
                    case QueueItem::HIGHEST:
                        prio_str = tr("Highest");
                        break;
                    default:
                        prio_str = tr("Normal");
                }

                return prio_str;
            }

            return item->data(index.column());
        }
        case Qt::TextAlignmentRole:
        {
            if (index.column() == COLUMN_DOWNLOADQUEUE_SIZE ||
                index.column() == COLUMN_DOWNLOADQUEUE_ESIZE ||
                index.column() == COLUMN_DOWNLOADQUEUE_DOWN)
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            else
                return static_cast<int>(Qt::AlignLeft);
        }
        case Qt::ForegroundRole:
        {
            QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();

            if (!errors.isEmpty() && errors != tr("No errors"))
                return AppTheme::errorColor();

            break;
        }
        case Qt::BackgroundColorRole:
            break;
        case Qt::ToolTipRole:
        {
            if (item->dir)
                break;

            QString added  = item->data(COLUMN_DOWNLOADQUEUE_ADDED).toString();
            QString errors = item->data(COLUMN_DOWNLOADQUEUE_ERR).toString();
            QString path   = item->data(COLUMN_DOWNLOADQUEUE_PATH).toString();

            if (errors.isEmpty())
                errors = tr("No errors");

            QString tooltip = QString(tr("<b>Added: </b> %1\n"
                                         "<b>Path: </b> %2\n"
                                         "<b>Errors: </b> %3\n")).arg(added).arg(path).arg(errors);

            return tooltip;
        }
    }

    return QVariant();
}

QVariant DownloadQueueModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Status") << tr("Size") << tr("Downloaded")
             << tr("Priority") << tr("User") << tr("Path") << tr("Exact size")
             << tr("Errors") << tr("Added") << QString("TTH");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootData.at(section);

    return QVariant();
}

QModelIndex DownloadQueueModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DownloadQueueItem *parentItem;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<DownloadQueueItem*>(parent.internalPointer());

    DownloadQueueItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex DownloadQueueModel::parent(const QModelIndex &index) const
{
    Q_D(const static DownloadQueueModel);

    if (!index.isValid())
        return QModelIndex();

    DownloadQueueItem *childItem = static_cast<DownloadQueueItem*>(index.internalPointer());
    DownloadQueueItem *parentItem = childItem->parent();

    if (parentItem == d->rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int DownloadQueueModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const static DownloadQueueModel);
    DownloadQueueItem *parentItem;

    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<DownloadQueueItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void DownloadQueueModel::sort(int column, Qt::SortOrder order) {
    Q_D(DownloadQueueModel);

    d->sortColumn = column;
    d->sortOrder = order;

    if (!d->rootItem || d->rootItem->childItems.empty() || column < 0 || column > columnCount()-1)
        return;

    emit layoutAboutToBeChanged();

    sortDownloadQueueItems(column, order, d->rootItem);

    emit layoutChanged();
}

DownloadQueueItem *DownloadQueueModel::addItem(const QVariantMap &map){
    static quint64 counter = 0;

    DownloadQueueItem *droot = createPath(map["PATH"].toString());

    if (!droot)
        return nullptr;

    DownloadQueueItem *child = nullptr;
    QList<QVariant> childData;

    childData << map["FNAME"]
              << map["STATUS"]
              << (map["ESIZE"].toLongLong() > 0? map["ESIZE"] : 0)
              << (map["DOWN"].toLongLong() > 0? map["DOWN"] : 0)
              << map["PRIO"]
              << map["USERS"]
              << map["PATH"]
              << (map["ESIZE"].toLongLong() > 0? map["ESIZE"] : 0)
              << map["ERRORS"]
              << map["ADDED"]
              << map["TTH"];

    child = new DownloadQueueItem(childData, droot);
    droot->appendChild(child);

    Q_D(static DownloadQueueModel);

    d->total_files++;
    d->total_size += childData.at(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    emit updateStats(d->total_files, d->total_size);

    counter++;

    repaint();

    if ((counter % 100) == 0)
        QApplication::processEvents();

    return child;
}

void DownloadQueueModel::updItem(const QVariantMap &map){
    DownloadQueueItem *item = createPath(map["PATH"].toString());
    Q_D(static DownloadQueueModel);

    QString target_name = map["FNAME"].toString();
    DownloadQueueItem *target = findTarget(item, target_name);

    if (!target && !(target = addItem(map)))
        return;

    item = target;

    d->total_size -= item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    item->updateColumn(COLUMN_DOWNLOADQUEUE_STATUS, map["STATUS"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_DOWN, (map["DOWN"].toLongLong() > 0? map["DOWN"] : 0));
    item->updateColumn(COLUMN_DOWNLOADQUEUE_ESIZE, map["ESIZE"].toULongLong() > 0? map["ESIZE"] : 0);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_SIZE, map["ESIZE"].toULongLong() > 0? map["ESIZE"] : 0);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_PRIO, map["PRIO"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_USER, map["USERS"]);
    item->updateColumn(COLUMN_DOWNLOADQUEUE_ERR, map["ERRORS"]);

    d->total_size += item->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();

    emit updateStats(d->total_files, d->total_size);

    const QModelIndex idx = createIndexForItem(item);
    if (idx.isValid())
        emit dataChanged(idx, index(idx.row(), columnCount() - 1, idx.parent()));
}

bool DownloadQueueModel::remItem(const QVariantMap &map){
    DownloadQueueItem *item = createPath(map["PATH"].toString());

    if (item->childItems.size() < 1)
        return false;

    QString target_name = map["FNAME"].toString();
    DownloadQueueItem *target = findTarget(item, target_name);

    if (!target)
        return false;

    Q_D(static DownloadQueueModel);

    d->total_size -= target->data(COLUMN_DOWNLOADQUEUE_ESIZE).toULongLong();
    d->total_files--;

    if (item->childCount() > 1){
        beginRemoveRows(createIndexForItem(item), target->row(), target->row());
        {
            int r = target->row();

            item->childItems.removeAt(r);

            delete target;
        }
        endRemoveRows();
    }
    else {

        DownloadQueueItem *p = item;
        DownloadQueueItem *_t = nullptr;

        while (true) {
            if ((p == d->rootItem) || (p->childCount() > 1) || !p->parent())
                break;

            beginRemoveRows(createIndexForItem(p->parent()), p->row(), p->row());
            {
                p->parent()->childItems.removeAt(p->row());

                _t = p;
            }
            endRemoveRows();

            if (p->parent()->childCount() > 0)
                break;

            p = p->parent();

            delete _t;
        }
    }

    emit updateStats(d->total_files, d->total_size);

    return true;
}

DownloadQueueItem *DownloadQueueModel::getRootElem() const{
    Q_D(const DownloadQueueModel);

    return d->rootItem;
}

int DownloadQueueModel::getSortColumn() const {
    Q_D(const DownloadQueueModel);

    return d->sortColumn;
}

void DownloadQueueModel::setSortColumn(int c) {
    Q_D(DownloadQueueModel);

    d->sortColumn = c;
}

Qt::SortOrder DownloadQueueModel::getSortOrder() const {
    Q_D(const DownloadQueueModel);

    return d->sortOrder;
}

void DownloadQueueModel::setSortOrder(Qt::SortOrder o) {
    Q_D(DownloadQueueModel);

    d->sortOrder = o;
}

QModelIndex DownloadQueueModel::createIndexForItem(DownloadQueueItem *item){
    Q_D(DownloadQueueModel);

    if (!d->rootItem || !item || item == d->rootItem)
        return QModelIndex();

    return createIndex(item->row(), 0, item);
}

DownloadQueueItem *DownloadQueueModel::createPath(const QString & path){
    Q_D(static DownloadQueueModel);

    if (!d->rootItem)
        return nullptr;

    QString _path = path;
    _path.replace("\\", "/");

    QStringList list = _path.split("/", WULFOR_SKIP_EMPTY);

    DownloadQueueItem *root = d->rootItem;

    bool found = false;

    for (int i = 0; i < list.size(); i++){
        found = false;

        for (const auto &item : root->childItems){
            if (!item->dir)
                continue;

            QString name = item->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

            if (name == list.at(i)){
                found = true;
                root = item;

                break;
            }
        }

        if (!found){
            static QString data = "";

            for (int j = i; j < list.size(); j++){
                QList<QVariant> rootData;
                rootData << list.at(j)  << data << data << data
                         << data << data << data << data
                         << data << data << data;

                DownloadQueueItem *item = new DownloadQueueItem(rootData);
                item->dir = true;

                root->appendChild(item);

                root = item;
            }

            emit layoutChanged();

            return root;
        }
    }

    return root;
}

void DownloadQueueModel::clear(){
    blockSignals(true);

    Q_D(DownloadQueueModel);

    qDeleteAll(d->rootItem->childItems);
    d->rootItem->childItems.clear();

    blockSignals(false);

    emit layoutChanged();
}

void DownloadQueueModel::repaint(){
    emit layoutChanged();
}

DownloadQueueItem *DownloadQueueModel::findTarget(const DownloadQueueItem *item, const QString &name){
    DownloadQueueItem *target = nullptr;

    for (const auto &i : item->childItems){
        if (i->data(COLUMN_DOWNLOADQUEUE_NAME).toString() == name){
            target = i;

            break;
        }
    }

    return target;
}
