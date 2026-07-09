/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"

#include <QMap>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QDir>
#include <QShortcut>

#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"
#include "ArenaWidgetFactory.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "dcpp/ClientManager.h"
#include "dcpp/User.h"

#if _DEBUG_QT_UI
#include <QtDebug>
#endif

using namespace dcpp;

DownloadQueue::DownloadQueue(QWidget *parent):
        QWidget(parent), d_ptr(new DownloadQueuePrivate())
{
    setupUi(this);

    init();

    QueueManager::getInstance()->addListener(this);

    setUnload(false);
}

DownloadQueue::~DownloadQueue(){
    save();

    QueueManager::getInstance()->removeListener(this);
    Q_D(DownloadQueue);

    delete d->menu;
    delete d_ptr;
}

void DownloadQueue::closeEvent(QCloseEvent *e){
    isUnload()? e->accept() : e->ignore();
}

void DownloadQueue::requestDelete(){
    if (!treeView_TARGET->hasFocus())
        return;

    QModelIndexList list = treeView_TARGET->selectionModel()->selectedRows(0);

    if (list.isEmpty())
        return;

    QList<DownloadQueueItem*> items;

    for (const auto &i : list){
        DownloadQueueItem *item = reinterpret_cast<DownloadQueueItem*>(i.internalPointer());

        if (!item)
            continue;

        if (item->dir)
            getChilds(item, items);
        else if (!items.contains(item))
            items.push_front(item);
    }

    QueueManager *QM = QueueManager::getInstance();
    for (const auto &i : items){
        QString target = i->data(COLUMN_DOWNLOADQUEUE_PATH).toString() + i->data(COLUMN_DOWNLOADQUEUE_NAME).toString();

        try {
            QM->remove(target.toStdString());
        }
        catch (const Exception &){}
    }
}

void DownloadQueue::init(){
    Q_D(DownloadQueue);

    d->queue_model = new DownloadQueueModel(this);

    d->delegate = new DownloadQueueDelegate(d->queue_model);

    treeView_TARGET->setItemDelegate(d->delegate);
    treeView_TARGET->setModel(d->queue_model);
    treeView_TARGET->setItemsExpandable(true);
    treeView_TARGET->setRootIsDecorated(true);
    treeView_TARGET->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_TARGET->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    label_STATS->hide();

    d->deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    d->deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(this, SIGNAL(coreAdded(VarMap)),            this, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreRemoved(VarMap)),          this, SLOT(remFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreSourcesUpdated(VarMap)),   this, SLOT(updateFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreStatusUpdated(VarMap)),    this, SLOT(updateFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreMoved(VarMap)),            this, SLOT(remFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreMoved(VarMap)),            this, SLOT(addFile(VarMap)), Qt::QueuedConnection);

    connect(d->deleteShortcut, SIGNAL(activated()), this, SLOT(requestDelete()));
    connect(treeView_TARGET, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    connect(treeView_TARGET->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu(QPoint)));
    connect(d->queue_model, SIGNAL(needExpand(QModelIndex)), treeView_TARGET, SLOT(expand(QModelIndex)));
    connect(d->queue_model, SIGNAL(rowRemoved(QModelIndex)), this, SLOT(slotCollapseRow(QModelIndex)));
    connect(d->queue_model, SIGNAL(updateStats(quint64,quint64)), this, SLOT(slotUpdateStats(quint64,quint64)));
    connect(pushButton_EXPAND,      SIGNAL(clicked()), treeView_TARGET, SLOT(expandAll()));
    connect(pushButton_COLLAPSE,    SIGNAL(clicked()), treeView_TARGET, SLOT(collapseAll()));

    d->menu = new Menu();

    setAttribute(Qt::WA_DeleteOnClose);

    load();

    loadList();

    treeView_TARGET->expandAll();

    ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
}

void DownloadQueue::load(){
    WulforUtil::restoreTreeHeader(treeView_TARGET->header(), WVGET(WS_DQUEUE_STATE, QByteArray()).toByteArray());
    treeView_TARGET->setSortingEnabled(true);
}

void DownloadQueue::save(){
    WVSET(WS_DQUEUE_STATE, treeView_TARGET->header()->saveState());
}

