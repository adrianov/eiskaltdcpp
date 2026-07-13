/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "TransferViewDelegate.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"
#include "WulforSettings.h"

#include "dcpp/QueueManager.h"
#include "dcpp/DownloadManager.h"
#include "dcpp/UploadManager.h"
#include "dcpp/ConnectionManager.h"

TransferView::TransferView(QWidget *parent):
        QWidget(parent),
        model(nullptr)
{
    setupUi(this);

    init();

    QueueManager::getInstance()->addListener(this);
    DownloadManager::getInstance()->addListener(this);
    UploadManager::getInstance()->addListener(this);
    ConnectionManager::getInstance()->addListener(this);
}

TransferView::~TransferView(){
    QueueManager::getInstance()->removeListener(this);
    DownloadManager::getInstance()->removeListener(this);
    UploadManager::getInstance()->removeListener(this);
    ConnectionManager::getInstance()->removeListener(this);

    delete model;
}

void TransferView::closeEvent(QCloseEvent *e){
    save();

    e->accept();
}

void TransferView::resizeEvent(QResizeEvent *e){
    e->accept();

    if (isVisible())
        WISET(WI_TRANSFER_HEIGHT, height());
}

void TransferView::hideEvent(QHideEvent *e){
    save();

    WISET(WI_TRANSFER_HEIGHT, height());

    e->accept();
}

void TransferView::save(){
    WSSET(WS_TRANSFERS_STATE, treeView_TRANSFERS->header()->saveState().toBase64());
}

void TransferView::load(){
    int h = WIGET(WI_TRANSFER_HEIGHT);

    if (h >= 0)
        resize(this->width(), h);

    WulforUtil::restoreTreeHeader(treeView_TRANSFERS->header(), QByteArray::fromBase64(WSGET(WS_TRANSFERS_STATE).toUtf8()));
}

QSize TransferView::sizeHint() const{
    int h = WIGET(WI_TRANSFER_HEIGHT);

    if (h > 0)
        return QSize(300, h);

    return QSize(300, 250);
}

void TransferView::init(){
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    model = new TransferViewModel();

    treeView_TRANSFERS->setModel(model);
    treeView_TRANSFERS->setItemDelegate(new TransferViewDelegate(this));
    treeView_TRANSFERS->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_TRANSFERS->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(treeView_TRANSFERS, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu(QPoint)));
    connect(treeView_TRANSFERS->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu(QPoint)));

    connect(this, SIGNAL(coreDMRequesting(VarMap)),     model, SLOT(initTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDMQueued(VarMap)),         model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDMStarting(VarMap)),       model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDMTick(VarMap)),           model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    // Parent aggregates only; avoid full-tree sort every second (speed column still updates via dataChanged).
    connect(this, SIGNAL(coreUpdateParents()),          model, SLOT(updateParents()), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDMComplete(VarMap)),       model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUpdateTransferPosition(VarMap,qint64)), model, SLOT(updateTransferPos(VarMap,qint64)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDMFailed(VarMap)),         model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    // Must be queued: ConnectionManager fires Added off the UI thread.
    connect(this, SIGNAL(coreCMAdded(VarMap)),          model, SLOT(addConnection(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreCMConnected(VarMap)),      model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreCMRemoved(VarMap)),        model, SLOT(removeTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreCMFailed(VarMap)),         model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreCMStatusChanged(VarMap)),  model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreQMFinished(VarMap)),       model, SLOT(finishParent(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreQMRemoved(VarMap)),        model, SLOT(removeQueueTarget(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreDownloadComplete(QString)), this, SLOT(downloadComplete(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUMStarting(VarMap)),       model, SLOT(initTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUMTick(VarMap)),           model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUMComplete(VarMap)),       model, SLOT(updateTransfer(VarMap)), Qt::QueuedConnection);

    load();
}

void TransferView::slotHeaderMenu(const QPoint &){
    WulforUtil::headerMenu(treeView_TRANSFERS);
}
