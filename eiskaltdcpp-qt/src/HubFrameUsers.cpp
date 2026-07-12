/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrame.h"
#include "HubFramePrivate.h"
#include "MainWindow.h"
#include "Notification.h"
#include "PMWindow.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/Exception.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/OnlineUser.h"
#include "dcpp/QueueManager.h"
#include "dcpp/UploadManager.h"

using namespace dcpp;

void HubFrame::userUpdated(const UserPtr &user, const dcpp::Identity &id){
    Q_D(HubFrame);

    static WulforSettings *WS       = WulforSettings::getInstance();
    static bool showFavJoinsOnly    = WS->getBool(WB_CHAT_SHOW_JOINS_FAV);
    static bool showJoins           = WS->getBool(WB_CHAT_SHOW_JOINS);
    const  bool isFavorite          = FavoriteManager::getInstance()->isFavoriteUser(user);

    if (!d->model)
        return;

    UserListItem *item = d->model->itemForPtr(user);

    QString cid = _q(user->getCID().toBase32());

    if (item){
        d->total_shared -= item->getShare();

        d->model->updateUser(item, id, cid, isFavorite);
    }
    else{
        item = d->model->addUser(user, id, cid, isFavorite);

        QString nick = item->getNick();

        if (showJoins){
            do {
                if (showFavJoinsOnly && !isFavorite)
                    break;

                addStatus(nick + tr(" joins the chat"));
            } while (false);
        }

        if (isFavorite)
            Notification::getInstance()->showMessage(Notification::FAVORITE, tr("Favorites"), tr("%1 is now online").arg(nick));

        if (d->pm.contains(nick)){
            PMWindow *wnd = d->pm[nick];

            wnd->cid = cid;
            wnd->plainTextEdit_INPUT->setEnabled(true);
            wnd->hubUrl = _q(d->client->getHubUrl());

            d->pm.insert(cid, wnd);

            d->pm.remove(nick);

            pmUserEvent(cid, tr("User online."));
        }
    }

    d->total_shared += qlonglong(id.getBytesShared());
}

void HubFrame::userRemoved(const UserPtr &user, const dcpp::Identity &id){
    Q_UNUSED(id)
    Q_D(HubFrame);

    UserListItem *item = d->model->itemForPtr(user);

    if (!item)
        return;

    d->total_shared -= item->getShare();

    QString cid = item->getCID();
    QString nick = item->getNick();

    if (d->pm.contains(cid)){
        pmUserOffline(cid);

        PMWindow *pmw = d->pm[cid];

        d->pm.insert(nick, pmw);

        pmw->cid = nick;
        pmw->plainTextEdit_INPUT->setEnabled(false);//we need interface function

        d->pm.remove(cid);
    }

    if (WulforSettings::getInstance()->getBool(WB_CHAT_SHOW_JOINS)){
        do {
            if (WulforSettings::getInstance()->getBool(WB_CHAT_SHOW_JOINS_FAV) &&
                !FavoriteManager::getInstance()->isFavoriteUser(user))
                break;

            addStatus(nick + tr(" left the chat"));
        } while (false);
    }

    if (FavoriteManager::getInstance()->isFavoriteUser(user))
        Notification::getInstance()->showMessage(Notification::FAVORITE, tr("Favorites"), tr("%1 is now offline").arg(nick));

    d->model->removeUser(user);
}

void HubFrame::getParams(HubFrame::VarMap &map, const Identity &id){
    map["NICK"] = _q(id.getNick());
    map["SHARE"] = qlonglong(id.getBytesShared());
    map["COMM"] = _q(id.getDescription());
    map["TAG"] = _q(id.getTag());
    map["CONN"] = _q(id.getConnection());
    map["IP"] = _q(id.getIp());
    map["EMAIL"] = _q(id.getEmail());
    map["ISOP"] = id.isOp();
    map["SPEED"] = _q(id.getConnection());
    map["AWAY"] = id.isAway();
    map["CID"] = _q(id.getUser()->getCID().toBase32());
}

void HubFrame::browseUserFiles(const QString& id, bool match){
    string message;
    string cid = id.toStdString();

    if (!cid.empty()){
        try{
            UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

            if (user){
                Q_D(HubFrame);

                if (user == ClientManager::getInstance()->getMe())
                    MainWindow::getInstance()->browseOwnFiles();
                else if (match)
                    QueueManager::getInstance()->addList(HintedUser(user, d->client->getHubUrl()), QueueItem::FLAG_MATCH_QUEUE, "");
                else
                    QueueManager::getInstance()->addList(HintedUser(user, d->client->getHubUrl()), QueueItem::FLAG_CLIENT_VIEW, "");
            }
            else {
                message = QString(tr("User not found")).toStdString();
            }
        }
        catch (const Exception &e){
            message = e.getError();

            LogManager::getInstance()->message(message);
        }
    }
}

void HubFrame::grantSlot(const QString& id){
    Q_D(HubFrame);

    QString message = tr("User not found");

    if (!id.isEmpty()){
        UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));

        if (user){
            UploadManager::getInstance()->reserveSlot(HintedUser(user, d->client->getHubUrl()));
            message = tr("Slot granted to ") + WulforUtil::getInstance()->getNicks(user->getCID(), _q(d->client->getHubUrl()));
        }
    }

    MainWindow::getInstance()->setStatusMessage(message);
}

void HubFrame::silenceUser(const QString& id){
    Q_D(HubFrame);

    if (id.isEmpty() || !d->client)
        return;

    OnlineUser *ou = ClientManager::getInstance()->findOnlineUser(CID(_tq(id)), d->client->getHubUrl(), false);
    if (!ou || ou->getUser() == ClientManager::getInstance()->getMe())
        return;

    ou->getIdentity().setNoChat(!ou->getIdentity().noChat());
}

void HubFrame::addUserToFav(const QString& id){
    if (id.isEmpty())
        return;

    string cid = id.toStdString();

    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

    if (user){
        if (user != ClientManager::getInstance()->getMe() && !FavoriteManager::getInstance()->isFavoriteUser(user))
            FavoriteManager::getInstance()->addFavoriteUser(user);
    }
}

void HubFrame::delUserFromFav(const QString& id){
    if (id.isEmpty())
        return;

    string cid = id.toStdString();

    UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

    if (user){
        if (user != ClientManager::getInstance()->getMe() && FavoriteManager::getInstance()->isFavoriteUser(user))
            FavoriteManager::getInstance()->removeFavoriteUser(user);
    }
}

void HubFrame::changeFavStatus(const QString &id) {
    if (id.isEmpty())
        return;

    UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));

    if (user) {
        Q_D(HubFrame);

        UserListItem *item = nullptr;

        if (d->model)
            item = d->model->itemForPtr(user);

        bool bFav = FavoriteManager::getInstance()->isFavoriteUser(user);

        if (item) {
            QModelIndex ixb = d->model->index(item->row(), COLUMN_NICK);
            QModelIndex ixe = d->model->index(item->row(), COLUMN_EMAIL);

            d->model->repaintData(ixb, ixe);
        }

        QString message = WulforUtil::getInstance()->getNicks(id, _q(d->client->getHubUrl())) +
                (bFav ? tr(" has been added to favorites.") : tr(" has been removed from favorites."));

        MainWindow::getInstance()->setStatusMessage(message);
    }
}

void HubFrame::delUserFromQueue(const QString& id){
    if (!id.isEmpty()){
        UserPtr user = ClientManager::getInstance()->findUser(CID(id.toStdString()));

        if (user)
            QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
    }
}
