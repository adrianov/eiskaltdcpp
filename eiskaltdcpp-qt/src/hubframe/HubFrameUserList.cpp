/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrame.h"
#include "hubframe/HubFramePrivate.h"
#include "WulforUtil.h"
#include "hubframe/HubFrameMenu.h"
#include "Antispam.h"

#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>

using namespace dcpp;

QString HubFrame::getCIDforNick(QString nick){
    Q_D(HubFrame);

    return d->model->CIDforNick(nick, _q(d->client->getHubUrl()));
}

QString HubFrame::getUserInfo(UserListItem *item){
    Q_D(HubFrame);
    QString ttip = "";

    ttip += d->model->headerData(COLUMN_NICK, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getNick() + "\n";
    ttip += d->model->headerData(COLUMN_COMMENT, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getComment() + "\n";
    ttip += d->model->headerData(COLUMN_EMAIL, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getEmail() + "\n";
    ttip += d->model->headerData(COLUMN_IP, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getIP() + "\n";
    ttip += d->model->headerData(COLUMN_SHARE, Qt::Horizontal, Qt::DisplayRole).toString() + ": " +
            WulforUtil::formatBytes(item->getShare()) + "\n";
    ttip += d->model->headerData(COLUMN_TAG, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getTag() + "\n";
    ttip += d->model->headerData(COLUMN_CONN, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getConnection() + "\n";

    if (item->isOP())
        ttip += tr("Hub role: Operator");
    else
        ttip += tr("Hub role: User");

    if (item->isFav())
        ttip += tr("\nFavorite user");

    return ttip;
}

bool HubFrame::isOP(const QString& nick) {
    Q_D(HubFrame);

    UserListItem *item = d->model->itemForNick(nick, _q(d->client->getHubUrl()));

    return (item? item->isOP() : false);
}

bool HubFrame::hasCID(const dcpp::CID &cid, const QString &nick){
    Q_D(HubFrame);
    return (d->model->CIDforNick(nick, _q(d->client->getHubUrl())) == _q(cid.toBase32()));
}

void HubFrame::clearUsers(){
    Q_D(HubFrame);

    if (d->model){
        // Detach the view from the proxy before clearing its source.
        if (treeView_USERS->model() != d->model)
            treeView_USERS->setModel(d->model);
        if (d->proxy && d->proxy->sourceModel())
            d->proxy->setSourceModel(nullptr);
        d->model->blockSignals(true);
        d->model->clear();
        d->model->blockSignals(false);
    }

    d->total_shared = 0;

    treeView_USERS->repaint();

    slotUsersUpdated();

    d->model->repaint();
}

void HubFrame::slotUsersUpdated(){
    Q_D(HubFrame);

    QString text;
    if (d->proxy && treeView_USERS->model() == d->proxy){
        text = QString(tr("Users count: %3/%1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared))
                                 .arg(d->proxy->rowCount());
    }
    else {
        text = QString(tr("Users count: %1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared));
    }

    if (label_USERSTATE->text() != text)
        label_USERSTATE->setText(text);

    label_LAST_STATUS->setMaximumHeight(label_USERSTATE->height());
}

void HubFrame::slotFilterTextChanged(){
    QString text = lineEdit_FILTER->text();

    Q_D(HubFrame);

    if (!text.isEmpty()){
        if (!d->proxy){
            d->proxy = new UserListProxyModel();
            // Source model keeps its own order; dynamic resort from the proxy
            // reenters UserListModel::sort during endInsertRows and can crash.
            d->proxy->setDynamicSortFilter(false);
        }

        if (d->proxy->sourceModel() != d->model)
            d->proxy->setSourceModel(d->model);

        bool isRegExp = false;

        if (text.startsWith("##")){
            isRegExp = true;
            text.remove(0, 2);
        }

        if (!isRegExp){
            d->proxy->setFilterFixedString(text);
            d->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        }
        else{
            d->proxy->setFilterRegExp(text);
            d->proxy->setFilterCaseSensitivity(Qt::CaseSensitive);
        }

        d->proxy->setFilterKeyColumn(comboBox_COLUMNS->currentIndex());

        if (treeView_USERS->model() != d->proxy)
            treeView_USERS->setModel(d->proxy);
    }
    else {
        if (treeView_USERS->model() != d->model)
            treeView_USERS->setModel(d->model);
        // Detach so the idle proxy does not listen to insert/remove signals.
        if (d->proxy && d->proxy->sourceModel())
            d->proxy->setSourceModel(nullptr);
    }

    if (comboBox_COLUMNS->hasFocus())
        lineEdit_FILTER->setFocus();
}

template < QString (UserListItem::*func)() const >
static void copyTagToClipboard(QModelIndexList &list){
    QString ret = "";
    UserListItem *item = nullptr;

    for (const auto &i : list) {
        item = reinterpret_cast<UserListItem*> ( i.internalPointer() );

        if ( !ret.isEmpty() )
            ret += "\n";

        if ( item )
            ret += (item->*func)();
    }

    qApp->clipboard()->setText ( ret, QClipboard::Clipboard );
}

template < qulonglong (UserListItem::*func)() const >
static void copyTagToClipboard(QModelIndexList &list){
    QString ret = "";
    UserListItem *item = nullptr;

    for (const auto &i : list) {
        item = reinterpret_cast<UserListItem*> ( i.internalPointer() );

        if ( !ret.isEmpty() )
            ret += "\n";

        if ( item )
            ret += WulforUtil::formatBytes((item->*func)());
    }

    qApp->clipboard()->setText ( ret, QClipboard::Clipboard );
}

void HubFrame::slotUserListMenu(const QPoint&){
    QItemSelectionModel *selection_model = treeView_USERS->selectionModel();
    QModelIndexList proxy_list = selection_model->selectedRows(0);

    if (proxy_list.size() < 1)
        return;

    QString cid = "";

    Q_D(HubFrame);

    if (d->proxy && treeView_USERS->model() == d->proxy){
        QModelIndex i = d->proxy->mapToSource(proxy_list.at(0));
        cid = reinterpret_cast<UserListItem*>(i.internalPointer())->getCID();
    }
    else{
        QModelIndex i = proxy_list.at(0);
        cid = reinterpret_cast<UserListItem*>(i.internalPointer())->getCID();
    }

    HubFrameMenu::Action action = HubFrameMenu::getInstance()->execUserMenu(d->client, cid);
    UserListItem *item = nullptr;

    proxy_list = selection_model->selectedRows(0);

    if (proxy_list.size() < 1)
        return;

    QModelIndexList list;

    if (d->proxy && treeView_USERS->model() == d->proxy){
        for (const auto &i : proxy_list)
            list.push_back(d->proxy->mapToSource(i));
    }
    else
        list = proxy_list;

    switch (action){
        case HubFrameMenu::None:
        {
            return;
        }
        case HubFrameMenu::BrowseFilelist:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    browseUserFiles(item->getCID());
            }

            break;
        }
        case HubFrameMenu::PrivateMessage:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    addPM(item->getCID(), "", false);
            }

            break;
        }
        case HubFrameMenu::CopyText:
        {
            QString ttip = "";

            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    ttip += getUserInfo(item) + "\n";

                ttip += "\n";
            }

            if (!ttip.isEmpty())
                qApp->clipboard()->setText(ttip, QClipboard::Clipboard);

            break;
        }
        case HubFrameMenu::CopyNick:
        {
            copyTagToClipboard<&UserListItem::getNick> (list);

            break;
        }
        case HubFrameMenu::CopyComment:
        {
            copyTagToClipboard<&UserListItem::getComment> (list);

            break;
        }
        case HubFrameMenu::CopyIP:
        {
            copyTagToClipboard<&UserListItem::getIP> (list);

            break;
        }
        case HubFrameMenu::CopyShare:
        {
            copyTagToClipboard<&UserListItem::getShare> (list);

            break;
        }
        case HubFrameMenu::CopyTag:
        {
            copyTagToClipboard<&UserListItem::getTag> (list);

            break;
        }
        case HubFrameMenu::CopyEmail:
        {
            copyTagToClipboard<&UserListItem::getEmail> (list);

            break;
        }
        case HubFrameMenu::MatchQueue:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    browseUserFiles(item->getCID(), true);
            }

            break;
        }
        case HubFrameMenu::FavoriteAdd:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    addUserToFav(item->getCID());
            }

            break;
        }
        case HubFrameMenu::FavoriteRem:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    delUserFromFav(item->getCID());
            }

            break;
        }
        case HubFrameMenu::GrantSlot:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    grantSlot(item->getCID());
            }

            break;
        }
        case HubFrameMenu::RemoveQueue:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    delUserFromQueue(item->getCID());
            }

            break;
        }
        case HubFrameMenu::AntiSpamWhite:
        {

            if (AntiSpam::getInstance()){
                for (const auto &i : list){
                    item = reinterpret_cast<UserListItem*>(i.internalPointer());

                    (*AntiSpam::getInstance()) << eIN_WHITE << item->getNick();
                }
            }

            break;
        }
        case HubFrameMenu::AntiSpamBlack:
        {
            if (AntiSpam::getInstance()){
                for (const auto &i : list){
                    item = reinterpret_cast<UserListItem*>(i.internalPointer());

                    (*AntiSpam::getInstance()) << eIN_BLACK << item->getNick();
                }
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

