/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "QueuedUsers.h"
#include "WulforSettings.h"

#include "dcpp/UploadManager.h"
#include "dcpp/ClientManager.h"

#include <QMenu>

QueuedUsers::QueuedUsers(){
    setupUi(this);

    setUnload(false);

    model = new QueuedUsersModel(this);
    treeView_USERS->setModel(model);
    treeView_USERS->setContextMenuPolicy(Qt::CustomContextMenu);
    WulforUtil::restoreTreeHeader(treeView_USERS->header(), WVGET("queued-users/headerstate", QByteArray()).toByteArray());

    connect(this, SIGNAL(coreWaitingAddFile(VarMap)), this, SLOT(slotWaitingAddFile(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreWaitingRemoved(VarMap)), this, SLOT(slotWaitingRemoved(VarMap)), Qt::QueuedConnection);
    connect(treeView_USERS, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));

    UploadManager::getInstance()->addListener(this);

    ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
}

QueuedUsers::~QueuedUsers(){
    UploadManager::getInstance()->removeListener(this);
}

void QueuedUsers::closeEvent(QCloseEvent *e){
    if (isUnload()){
        WVSET("queued-users/headerstate", treeView_USERS->header()->saveState());

        e->accept();
    }
    else {
        e->ignore();
    }
}

void QueuedUsers::slotWaitingAddFile(const VarMap &map){
    model->addResult(map);
}

void QueuedUsers::slotWaitingRemoved(const VarMap &map){
    model->remResult(map);
}

void QueuedUsers::slotContextMenu(){
    QModelIndexList indexes = treeView_USERS->selectionModel()->selectedRows(0);

    if (indexes.isEmpty())
        return;

    QMenu *m = new QMenu(this);
    m->addAction(tr("Grant slot"));

    if (m->exec(QCursor::pos())){
        for (const auto &i : indexes){
            QueuedUserItem *item = reinterpret_cast<QueuedUserItem*>(i.internalPointer());

            if (!item)
                continue;

            QString id = item->cid;

            if (!id.isEmpty()){
                UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));

                if (user){
                    try { UploadManager::getInstance()->reserveSlot(HintedUser(user, _tq(item->hub))); }
                    catch ( ... ) {}
                }
            }

        }
    }

    m->deleteLater();
}

void QueuedUsers::on(WaitingAddFile, const dcpp::HintedUser &user, const std::string &file) noexcept {
    VarMap map;
    map["CID"]  = _q(user.user->getCID().toBase32());
    map["FILE"] = _q(file);
    map["HUB"]  = _q(user.hint);

    emit coreWaitingAddFile(map);
}

void QueuedUsers::on(WaitingRemoveUser, const dcpp::HintedUser &user) noexcept {
    VarMap map;
    map["CID"]  = _q(user.user->getCID().toBase32());
    map["FILE"] = "";
    map["HUB"]  = _q(user.hint);

    emit coreWaitingRemoved(map);
}
