/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"
#include "Notification.h"
#include "SearchFrame.h"
#include "ArenaWidgetFactory.h"

#include "dcpp/CID.h"
#include "dcpp/ClientManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/UploadManager.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/HashManager.h"

void TransferView::getFileList(const QString &cid, const QString &host){
    if (cid.isEmpty() || host.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            QueueManager::getInstance()->addList(HintedUser(user, _tq(host)), QueueItem::FLAG_CLIENT_VIEW, "");
    }
    catch (const Exception&){}
}

void TransferView::matchQueue(const QString &cid, const QString &host){
    if (cid.isEmpty() || host.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            QueueManager::getInstance()->addList(HintedUser(user, _tq(host)), QueueItem::FLAG_MATCH_QUEUE, "");
    }
    catch (const Exception&){}
}

void TransferView::addFavorite(const QString &cid){
    if (cid.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            FavoriteManager::getInstance()->addFavoriteUser(user);
    }
    catch (const Exception&){}
}

void TransferView::grantSlot(const QString &cid, const QString &host){
    if (cid.isEmpty() || host.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            UploadManager::getInstance()->reserveSlot(HintedUser(user, _tq(host)));
    }
    catch (const Exception&){}
}

void TransferView::removeFromQueue(const QString &cid){
    if (cid.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
    }
    catch (const Exception&){}
}

void TransferView::removeTransfer(const QString &target){
    if (target.isEmpty())
        return;

    try {
        QueueManager::getInstance()->remove(_tq(target));
    }
    catch (const Exception&){}
}

void TransferView::forceAttempt(const QString &cid){
    if (cid.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            ConnectionManager::getInstance()->force(user);
    }
    catch (const Exception&){}
}

void TransferView::closeConection(const QString &cid, bool download){
    if (cid.isEmpty())
        return;

    try{
        dcpp::UserPtr user = ClientManager::getInstance()->getUser(CID(_tq(cid)));

        if (user)
            ConnectionManager::getInstance()->disconnect(user, download);
    }
    catch (const Exception&){}
}

void TransferView::searchAlternates(const QString &tth){
    if (tth.isEmpty())
        return;

    SearchFrame *sfr = ArenaWidgetFactory().create<SearchFrame>();
    sfr->searchAlternates(tth);
}

void TransferView::downloadComplete(QString target){
    Notification::getInstance()->showMessage(Notification::TRANSFER, tr("Download complete"), target);
}

QString TransferView::getTTHFromItem(const TransferViewItem *item){
    QString tth_str = "";

    if (item->download)
        tth_str = item->tth;
    else {
        const TTHValue *tth = dcpp::HashManager::getInstance()->getFileTTHif(_tq(item->target));

        if (tth)
            tth_str = _q(tth->toBase32());
    }

    return tth_str;
}
