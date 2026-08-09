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

#include "mainwindow/WindowSetup.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "ArenaWidgetManager.h"
#include "ShareIndexListListener.h"
#include "VersionGlobal.h"
#include "WulforUtil.h"
#include "appicon/AppIcons.h"
#include "ArenaWidget.h"

#include "dcpp/SettingsManager.h"

#ifdef LUA_SCRIPT
#include "dcpp/ScriptManager.h"
#endif

#include <QApplication>
#include <QDockWidget>
#include <QScreen>

using namespace dcpp;

void WindowSetup::run()
{
    host->setObjectName("MainWindow");

    QObject::connect(host, SIGNAL(coreLogMessage(QString)), host, SLOT(setStatusMessage(QString)), Qt::QueuedConnection);
    QObject::connect(host, SIGNAL(coreUpdateStats(QMap<QString,QString>)), host, SLOT(updateStatus(QMap<QString,QString>)), Qt::QueuedConnection);

    if (ShareIndexListListener::getInstance()) {
        QObject::connect(ShareIndexListListener::getInstance(),
                         SIGNAL(openShare(dcpp::UserPtr,QString,QString)),
                         host, SLOT(showShareBrowser(dcpp::UserPtr,QString,QString)), Qt::QueuedConnection);
        QObject::connect(ShareIndexListListener::getInstance(), SIGNAL(queueEmpty()),
                         host, SLOT(slotShareIndexQueueEmpty()), Qt::QueuedConnection);
    }

    d->arena = new QDockWidget();
    d->arena->setWidget(nullptr);
    d->arena->setFloating(false);
    d->arena->setContentsMargins(0, 0, 0, 0);
    d->arena->setAllowedAreas(Qt::RightDockWidgetArea);
    d->arena->setFeatures(QDockWidget::NoDockWidgetFeatures);
    d->arena->setContextMenuPolicy(Qt::CustomContextMenu);
    d->arena->setTitleBarWidget(new QWidget(d->arena));
    d->arena->setMinimumSize(10, 10);

    d->transfer_dock = new QDockWidget(host);
    d->transfer_dock->setWidget(nullptr);
    d->transfer_dock->setFloating(false);
    d->transfer_dock->setObjectName("transfer_dock");
    d->transfer_dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    d->transfer_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    d->transfer_dock->setContextMenuPolicy(Qt::CustomContextMenu);
    d->transfer_dock->setTitleBarWidget(new QWidget(d->transfer_dock));
    d->transfer_dock->setMinimumSize(QSize(8, 8));

    host->setCentralWidget(d->arena);
    host->addDockWidget(Qt::BottomDockWidgetArea, d->transfer_dock);
    d->transfer_dock->hide();

    host->setWindowIcon(WICON(AppIcons::eiICON_APPL));
    host->setWindowTitle(QString::fromStdString(eiskaltdcppAppNameString));

    host->initActions();
    host->initMenuBar();
#if defined(Q_OS_MAC)
    host->initDockMenuBar();
#endif
    host->initStatusBar();
    host->initSearchBar();
    host->initToolbar();
    host->initSideBar();

    WulforUtil *WU = WulforUtil::getInstance();
    QObject::connect(WU, SIGNAL(iconsLoaded()), host, SLOT(updateActionIcons()));
    QObject::connect(qApp, SIGNAL(screenAdded(QScreen*)), WU, SLOT(loadIcons()));
    QObject::connect(qApp, SIGNAL(screenRemoved(QScreen*)), WU, SLOT(loadIcons()));
    QObject::connect(qApp, SIGNAL(primaryScreenChanged(QScreen*)), WU, SLOT(loadIcons()));

    host->loadSettings();
    QObject::connect(qApp, SIGNAL(aboutToQuit()), host, SLOT(slotExit()));

    QObject::connect(ArenaWidgetManager::getInstance(), SIGNAL(activated(ArenaWidget*)), host, SLOT(mapWidgetOnArena(ArenaWidget*)));
    QObject::connect(ArenaWidgetManager::getInstance(), SIGNAL(added(ArenaWidget*)), host, SLOT(insertWidget(ArenaWidget*)));
    QObject::connect(ArenaWidgetManager::getInstance(), SIGNAL(removed(ArenaWidget*)), host, SLOT(removeWidget(ArenaWidget*)));
    QObject::connect(ArenaWidgetManager::getInstance(), SIGNAL(updated(ArenaWidget*)), host, SLOT(updated(ArenaWidget*)));

#ifdef LUA_SCRIPT
    ScriptManager::getInstance()->load();
    if (BOOLSETTING(USE_LUA))
        ScriptManager::getInstance()->EvaluateFile("startup.lua");
#endif
}
