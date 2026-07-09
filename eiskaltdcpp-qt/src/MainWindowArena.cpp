/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MainWindow.h"
#include "MainWindowPrivate.h"
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
#include "QueuedUsers.h"

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
