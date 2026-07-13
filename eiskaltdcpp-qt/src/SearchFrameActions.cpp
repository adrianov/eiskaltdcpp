/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "MainWindow.h"
#include "WulforUtil.h"

#include "dcpp/QueueManager.h"
#include "dcpp/UploadManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/User.h"

using namespace dcpp;

void SearchFrame::download(const SearchFrame::VarMap &params){
    string target, cid, filename, hubUrl;
    int64_t size;

    target      = params["TARGET"].toString().toStdString();
    cid         = params["CID"].toString().toStdString();
    filename    = params["FNAME"].toString().toStdString();
    hubUrl      = params["HOST"].toString().toStdString();
    size        = (int64_t)params["ESIZE"].toLongLong();

    if (cid.empty())
        return;

    try{
        // getUser: ShareIndex / GC'd peers are not always in the live user map.
        UserPtr user = ClientManager::getInstance()->getUser(CID(cid));
        // Only files have a TTH
        if (!params["TTH"].toString().isEmpty()){
            string subdir = params["FNAME"].toString().split("\\", WULFOR_SKIP_EMPTY).last().toStdString();
            QueueManager::getInstance()->add(target + subdir, size, TTHValue(params["TTH"].toString().toStdString()), HintedUser(user, hubUrl));
        }
        else{
            QueueManager::getInstance()->addDirectory(filename, HintedUser(user, hubUrl), target);
        }
    }
    catch (const Exception &e){
        MainWindow::getInstance()->setStatusMessage(_q(e.getError()));
    }
}

void SearchFrame::getFileList(const VarMap &params, bool match){
    string cid  = params["CID"].toString().toStdString();
    string dir  = params["FNAME"].toString().toStdString();
    string host = params["HOST"].toString().toStdString();

    if (cid.empty())
        return;

    try {
        UserPtr user = ClientManager::getInstance()->getUser(CID(cid));
        QueueItem::FileFlags flag = match? QueueItem::FLAG_MATCH_QUEUE : QueueItem::FLAG_CLIENT_VIEW;
        QueueManager::getInstance()->addList(HintedUser(user, host), flag, dir);
    }
    catch (const Exception &e){
        MainWindow::getInstance()->setStatusMessage(_q(e.getError()));
    }
}

void SearchFrame::addToFav(const QString &cid){
    if (!cid.isEmpty()){
        try {
            UserPtr user = ClientManager::getInstance()->findUser(CID(cid.toStdString()));

            if (user)
                FavoriteManager::getInstance()->addFavoriteUser(user);
        }
        catch (const Exception&){}
    }
}

void SearchFrame::grant(const VarMap &params){
    string cid  = params["CID"].toString().toStdString();
    string host = params["HOST"].toString().toStdString();

    if (cid.empty())
        return;

    try {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

        if (user)
            UploadManager::getInstance()->reserveSlot(HintedUser(user, host));
    }
    catch (const Exception&){}
}

void SearchFrame::removeSource(const VarMap &params){
    string cid  = params["CID"].toString().toStdString();

    if (cid.empty())
        return;

    try {
        UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

        if (user)
            QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
    }
    catch (const Exception&){}

}

void SearchFrame::onHubAdded(const QString &info){
    Q_D(SearchFrame);

    if (d->hubs.contains(info) || info.isEmpty())
        return;

    d->hubs.push_back(info);
    d->client_list.push_back(ClientManager::getInstance()->getClient(_tq(info)));

    d->str_model->setStringList(d->hubs);
}

void SearchFrame::onHubChanged(const QString &info){
    Q_D(SearchFrame);

    if (!d->hubs.contains(info) || info.isEmpty())
        return;

    Client *cl = ClientManager::getInstance()->getClient(_tq(info));
    if (!cl || d->client_list.indexOf(cl) < 0)
        return;

    d->hubs.removeAt(d->client_list.indexOf(cl));
    d->client_list.removeAt(d->client_list.indexOf(cl));

    d->hubs.push_back(info);
    d->client_list.push_back(cl);

    d->str_model->setStringList(d->hubs);
}

void SearchFrame::onHubRemoved(const QString &info){
    Q_D(SearchFrame);

    if (!d->hubs.contains(info) || info.isEmpty())
        return;

    d->client_list.removeAt(d->hubs.indexOf(info));
    d->hubs.removeAt(d->hubs.indexOf(info));

    d->str_model->setStringList(d->hubs);
}

