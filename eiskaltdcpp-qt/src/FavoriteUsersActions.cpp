/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FavoriteUsers.h"
#include "FavoriteUsersModel.h"
#include "WulforUtil.h"

#include <QMenu>
#include <QInputDialog>

#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/User.h"
#include "dcpp/CID.h"
#include "dcpp/Util.h"

using namespace dcpp;

bool FavoriteUsers::addUserToFav(const QString &id){
    if (id.isEmpty())
        return false;

    UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));
    if (user && user != ClientManager::getInstance()->getMe()
            && !FavoriteManager::getInstance()->isFavoriteUser(user))
        FavoriteManager::getInstance()->addFavoriteUser(user);

    return true;
}

bool FavoriteUsers::remUserFromFav(const QString &id){
    if (id.isEmpty())
        return false;

    UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));
    if (user && user != ClientManager::getInstance()->getMe()
            && FavoriteManager::getInstance()->isFavoriteUser(user))
        FavoriteManager::getInstance()->removeFavoriteUser(user);

    return true;
}

void FavoriteUsers::handleRemove(const QString &cid){
    UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    if (user)
        FavoriteManager::getInstance()->removeFavoriteUser(user);
}

void FavoriteUsers::handleDesc(const QString &cid){
    FavoriteUserItem *item = model->itemForCID(cid);
    static QString old;

    if (!item)
        return;

    UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    if (!user)
        return;

    QString desc = QInputDialog::getText(this, item->data(COLUMN_USER_NICK).toString(),
                                         tr("Description"), QLineEdit::Normal, old);
    if (desc.isEmpty())
        return;

    old = desc;
    item->updateColumn(COLUMN_USER_DESC, desc);
    FavoriteManager::getInstance()->setUserDescription(user, _tq(desc));
}

void FavoriteUsers::getFileList(const QString &cid){
    if (cid.isEmpty())
        return;

    UserPtr user = ClientManager::getInstance()->findUser(CID(_tq(cid)));
    if (!user)
        return;

    // HUB column shows display names; browse needs a real hub URL hint.
    string hub;
    StringList hubs = ClientManager::getInstance()->getHubs(user->getCID(), Util::emptyString);
    if (!hubs.empty())
        hub = hubs[0];
    else
        hub = FavoriteManager::getInstance()->getUserURL(user);

    try {
        QueueManager::getInstance()->addList(HintedUser(user, hub), QueueItem::FLAG_CLIENT_VIEW, "");
    } catch (const Exception&) {
    }
}

void FavoriteUsers::handleBrowseShare(const QString &cid){
    getFileList(cid);
}

void FavoriteUsers::handleGrant(const QString &cid){
    FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
    auto i = ul.find(CID(_tq(cid)));
    if (i == ul.end())
        return;

    FavoriteUser &u = i->second;
    if (_q(u.getUser()->getCID().toBase32()) != cid)
        return;

    const bool grant = !u.isSet(FavoriteUser::FLAG_GRANTSLOT);
    if (grant)
        u.setFlag(FavoriteUser::FLAG_GRANTSLOT);
    else
        u.unsetFlag(FavoriteUser::FLAG_GRANTSLOT);
    FavoriteManager::getInstance()->setAutoGrant(u.getUser(), grant);
}

void FavoriteUsers::slotContextMenu(){
    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    QList<FavoriteUserItem*> items;

    for (const auto &i : indexes)
        items.push_back(reinterpret_cast<FavoriteUserItem*>(i.internalPointer()));

    if (items.isEmpty())
        return;

    QMenu *menu = new QMenu(this);
    menu->deleteLater();

    QAction *remove = new QAction(tr("Remove"), menu);
    remove->setIcon(WICON(WulforUtil::eiEDITDELETE));

    QAction *desc = new QAction(tr("Description"), menu);
    desc->setIcon(WICON(WulforUtil::eiEDIT));

    QAction *grant = new QAction(tr("Grant/Remove slot"), menu);
    grant->setIcon(WICON(WulforUtil::eiBALL_GREEN));

    QAction *browse = new QAction(tr("Browse Files"), menu);
    browse->setIcon(WICON(WulforUtil::eiFOLDER_BLUE));

    menu->addActions(QList<QAction*>() << browse << desc << grant << remove);

    QAction *ret = menu->exec(QCursor::pos());
    if (!ret)
        return;

    for (const auto &i : items) {
        if (ret == remove)
            handleRemove(i->cid);
        else if (ret == grant)
            handleGrant(i->cid);
        else if (ret == browse)
            handleBrowseShare(i->cid);
        else
            handleDesc(i->cid);
    }
}
