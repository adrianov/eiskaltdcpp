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

#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "WulforUtil.h"
#include "PMWindow.h"
#include "ArenaWidgetManager.h"
#include "Magnet.h"
#include "HubManager.h"
#include "HubFrame.h"
#include "ArenaWidgetFactory.h"
#include "DownloadQueue.h"
#include "FinishedTransfers.h"
#include "FavoriteHubs.h"
#include "FavoriteUsers.h"
#include "PublicHubs.h"
#include "SpyFrame.h"
#include "ADLS.h"
#include "CmdDebug.h"
#include "Secretary.h"
#include "queuedusers/QueuedUsers.h"

#include "dcpp/FavoriteManager.h"
#include "dcpp/SettingsManager.h"

#include <typeinfo>
#include <QUrl>
#include <QToolButton>
#include <QToolBar>

using namespace dcpp;

void MainWindow::initFavHubMenu() {
    Q_D(MainWindow);

    if (!d->fBar)
        return;

    if (!d->favHubMenu) {
        d->favHubMenu = new QMenu(this);

        connect(d->favHubMenu, SIGNAL(aboutToShow()), this, SLOT(slotUpdateFavHubMenu()));
        connect(d->favHubMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotConnectFavHub(QAction*)));
    }

    QToolButton * btn = qobject_cast<QToolButton *>(d->fBar->widgetForAction(d->hubsFavoriteHubs));
    if (btn) {
        btn->setMenu(d->favHubMenu);
        btn->setPopupMode(QToolButton::MenuButtonPopup);
    }
}

QObject *MainWindow::getToolBar(){
    Q_D(MainWindow);

    if (!d->fBar)
        return nullptr;

    return qobject_cast<QObject*>(reinterpret_cast<QToolBar*>(d->fBar->qt_metacast("QToolBar")));
}

ArenaWidget *MainWindow::widgetForRole(ArenaWidget::Role r) const{
    ArenaWidget *awgt = nullptr;
    Q_D(const MainWindow);

    switch (r){
    case ArenaWidget::Downloads:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, DownloadQueue>();
            awgt->setToolButton(d->toolsDownloadQueue);

            break;
        }
    case ArenaWidget::FinishedUploads:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, FinishedUploads>();
            awgt->setToolButton(d->toolsFinishedUploads);

            break;
        }
    case ArenaWidget::FinishedDownloads:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, FinishedDownloads>();
            awgt->setToolButton(d->toolsFinishedDownloads);

            break;
        }
    case ArenaWidget::FavoriteHubs:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, FavoriteHubs>();
            awgt->setToolButton(d->hubsFavoriteHubs);

            break;
        }
    case ArenaWidget::FavoriteUsers:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, FavoriteUsers>();
            awgt->setToolButton(d->hubsFavoriteUsers);

            break;
        }
    case ArenaWidget::PublicHubs:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, PublicHubs>();
            awgt->setToolButton(d->hubsPublicHubs);

            break;
        }
    case ArenaWidget::SearchSpy:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, SpyFrame>();
            awgt->setToolButton(d->toolsSearchSpy);

            break;
        }
    case ArenaWidget::ADLS:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, ADLS>();
            awgt->setToolButton(d->toolsADLS);

            break;
        }
    case ArenaWidget::CmdDebug:
    {
        awgt = ArenaWidgetFactory().create<dcpp::Singleton, CmdDebug>();
        awgt->setToolButton(d->toolsCmdDebug);

        break;
    }
    case ArenaWidget::Secretary:
    {
        awgt = ArenaWidgetFactory().create<dcpp::Singleton, Secretary>();
        awgt->setToolButton(d->toolsSecretary);

        break;
    }
    case ArenaWidget::QueuedUsers:
        {
            awgt = ArenaWidgetFactory().create<dcpp::Singleton, QueuedUsers>();
            awgt->setToolButton(d->toolsQueuedUsers);

            break;
        }
    default:
        break;
    }

    return awgt;
}

void MainWindow::newHubFrame(QString address, QString enc){
    if (address.isEmpty())
        return;

    address = QUrl::fromPercentEncoding(address.toUtf8());

    HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(address));

    if (fr){
        ArenaWidgetManager::getInstance()->activate(fr);

        return;
    }

    fr = ArenaWidgetFactory().create<HubFrame, QWidget*, QString, QString>(this, address, enc);

    ArenaWidgetManager::getInstance()->activate(fr);
}

void MainWindow::autoconnect(){
    const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();

    for (const auto &i : fl) {
        FavoriteHubEntry* entry = i;

        if (entry->getConnect()) {
            if (entry->getNick().empty() && SETTING(NICK).empty())
                continue;

            QString encoding = WulforUtil::getInstance()->dcEnc2QtEnc(QString::fromStdString(entry->getEncoding()));

            newHubFrame(QString::fromStdString(entry->getServer()), encoding);
        }
    }
}

void MainWindow::parseCmdLine(const QStringList &args){
    for (const auto &arg : args){
        if (arg.startsWith("magnet:?")){
            Magnet m(this);
            m.setLink(arg);
            m.exec();
        }
        else if (arg.startsWith("dchub://") || arg.startsWith("nmdcs://")){
            newHubFrame(arg, "");
        }
        else if (arg.startsWith("adc://") || arg.startsWith("adcs://")){
            newHubFrame(arg, "UTF-8");
        }
    }
}

void MainWindow::parseInstanceLine(const QString &data){
    if (!isVisible()){
        show();
        raise();

        redrawToolPanel();
    }

    const QStringList args = data.split("\n", WULFOR_SKIP_EMPTY);
    parseCmdLine(args);
}

void MainWindow::insertWidget ( ArenaWidget* awgt ) {
    if (!awgt || (awgt && (awgt->state() & ArenaWidget::Hidden)))
        return;

    Q_D(MainWindow);

    QAction *act = d->menuWidgets->addAction(awgt->getPixmap(), awgt->getArenaShortTitle());

    d->menuWidgetsHash.insert(act, awgt);

    connect(act, SIGNAL(triggered(bool)), this, SLOT(slotWidgetsToggle()));
}

void MainWindow::removeWidget ( ArenaWidget* awgt ) {
    Q_D(MainWindow);

    QAction *act = d->menuWidgetsHash.key(awgt);

    if (!act)
        return;

    d->menuWidgetsHash.remove(act);

    act->deleteLater();
}

void MainWindow::updated ( ArenaWidget* awgt ) {
    if (!awgt)
        return;

    if (awgt->state() & ArenaWidget::Hidden)
        removeWidget(awgt);
    else
        insertWidget(awgt);
}

void MainWindow::slotUpdateFavHubMenu() {
    Q_D(MainWindow);

    d->favHubMenu->clear();

    const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();

    for (auto &i : fl) {
        const FavoriteHubEntry &entry = *i;

        QString url = _q(entry.getServer());
        QString name = entry.getName().empty() ? tr("[No name]") : _q(entry.getName());
        QString encoding = WulforUtil::getInstance()->dcEnc2QtEnc(QString::fromStdString(entry.getEncoding()));
        QString menuItem = QString("%1 - %2").arg(name).arg(url);

        QAction *action = new QAction(menuItem, d->favHubMenu);
        action->setStatusTip(encoding);
        action->setToolTip(url);

        if (qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(url))) {
            action->setCheckable(true);
            action->setChecked(true);
        }

        d->favHubMenu->addAction(action);
    }
}

void MainWindow::slotConnectFavHub(QAction *action) {

    QString url = action->toolTip();
    QString encoding = action->statusTip();

    newHubFrame(url, encoding);
}

void MainWindow::nextMsg(){
    Q_D(MainWindow);

    HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->activeHub());

    if (fr)
        fr->nextMsg();
    else{
        QWidget *wg = d->arena->widget();

        bool pmw = false;

        if (wg)
            pmw = (typeid(*wg) == typeid(PMWindow));

        if(pmw){
            PMWindow *pm = qobject_cast<PMWindow *>(wg);

            if (pm)
                pm->nextMsg();
        }
    }
}

void MainWindow::prevMsg(){
    Q_D(MainWindow);
    HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->activeHub());

    if (fr)
        fr->prevMsg();
    else{
        QWidget *wg = d->arena->widget();

        bool pmw = false;

        if (wg)
            pmw = (typeid(*wg) == typeid(PMWindow));

        if(pmw){
            PMWindow *pm = qobject_cast<PMWindow *>(wg);

            if (pm)
                pm->prevMsg();
        }
    }
}


