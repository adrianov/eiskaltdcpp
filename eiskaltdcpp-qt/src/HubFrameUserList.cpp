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
#include "WulforUtil.h"

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
        if (d->proxy && d->proxy->sourceModel())
            d->proxy->setSourceModel(nullptr);
        d->model->blockSignals(true);
        d->model->clear();
        d-> model->blockSignals(false);
        treeView_USERS->setModel(d->model);
    }

    d->total_shared = 0;

    treeView_USERS->repaint();

    slotUsersUpdated();

    d->model->repaint();
}

void HubFrame::slotUsersUpdated(){
    Q_D(HubFrame);

    if (d->proxy && treeView_USERS->model() == d->proxy){
        label_USERSTATE->setText(QString(tr("Users count: %3/%1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared))
                                 .arg(d->proxy->rowCount()));
    }
    else {
        label_USERSTATE->setText(QString(tr("Users count: %1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared)));
    }

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
