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

#include "mainwindow/actions/WindowActions.h"
#include "mainwindow/actions/ActionCatalog.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "WulforUtil.h"
#include "shortcut/ShortcutManager.h"
#include "appicon/AppIcons.h"

#include <QAction>
#include <QMenu>
#include <QObject>

void WindowActions::build()
{
    ActionCatalog(host, d).build();

    ShortcutManager *SM = ShortcutManager::getInstance();

    d->menuWidgets = new QMenu("", host);

    d->nextTabShortCut     = new  QAction(host);
    d->prevTabShortCut     = new  QAction(host);
    d->nextMsgShortCut     = new  QAction(host);
    d->prevMsgShortCut     = new  QAction(host);
    d->closeWidgetShortCut      = new  QAction(host);
    d->toggleMainMenuShortCut   = new  QAction(host);

    d->nextTabShortCut->setObjectName("nextTabShortCut");
    d->prevTabShortCut->setObjectName("prevTabShortCut");
    d->nextMsgShortCut->setObjectName("nextMsgShortCut");
    d->prevMsgShortCut->setObjectName("prevMsgShortCut");
    d->closeWidgetShortCut->setObjectName("closeWidgetShortCut");
    d->toggleMainMenuShortCut->setObjectName("toggleMainMenuShortCut");

    d->nextTabShortCut->setText(MainWindow::tr("Next widget"));
    d->prevTabShortCut->setText(MainWindow::tr("Previous widget"));
    d->nextMsgShortCut->setText(MainWindow::tr("Next message"));
    d->prevMsgShortCut->setText(MainWindow::tr("Previous message"));
    d->closeWidgetShortCut->setText(MainWindow::tr("Close current widget"));
    d->toggleMainMenuShortCut->setText(MainWindow::tr("Toggle main menu"));

    d->nextTabShortCut->setShortcutContext(Qt::ApplicationShortcut);
    d->prevTabShortCut->setShortcutContext(Qt::ApplicationShortcut);
    d->nextMsgShortCut->setShortcutContext(Qt::ApplicationShortcut);
    d->prevMsgShortCut->setShortcutContext(Qt::ApplicationShortcut);
    d->closeWidgetShortCut->setShortcutContext(Qt::ApplicationShortcut);
    d->toggleMainMenuShortCut->setShortcutContext(Qt::ApplicationShortcut);

    SM->registerShortcut(d->nextTabShortCut, QString("Ctrl+PgDown"));
    SM->registerShortcut(d->prevTabShortCut, QString("Ctrl+PgUp"));
    SM->registerShortcut(d->nextMsgShortCut, QString("Ctrl+Down"));
    SM->registerShortcut(d->prevMsgShortCut, QString("Ctrl+Up"));
    SM->registerShortcut(d->closeWidgetShortCut, QString("Ctrl+W"));
    SM->registerShortcut(d->toggleMainMenuShortCut, QString("Ctrl+M"));

    QObject::connect(d->nextMsgShortCut,        SIGNAL(triggered()), host, SLOT(nextMsg()));
    QObject::connect(d->prevMsgShortCut,        SIGNAL(triggered()), host, SLOT(prevMsg()));
    QObject::connect(d->closeWidgetShortCut,    SIGNAL(triggered()), host, SLOT(slotCloseCurrentWidget()));
    QObject::connect(d->toggleMainMenuShortCut, SIGNAL(triggered()), host, SLOT(slotHideMainMenu()));

    d->sh_menu = new QMenu(host);
    d->sh_menu->addActions(QList<QAction*>()
                        << d->nextTabShortCut
                        << d->prevTabShortCut
                        << d->nextMsgShortCut
                        << d->prevMsgShortCut
                        << d->closeWidgetShortCut
                        << d->toggleMainMenuShortCut);

    d->panelsWidgets = new QAction("", host);
    d->panelsWidgets->setCheckable(true);
    QObject::connect(d->panelsWidgets, SIGNAL(triggered()), host, SLOT(slotPanelMenuActionClicked()));

    d->panelsTools = new QAction("", host);
    d->panelsTools->setCheckable(true);
    QObject::connect(d->panelsTools, SIGNAL(triggered()), host, SLOT(slotPanelMenuActionClicked()));

    d->panelsSearch = new QAction("", host);
    d->panelsSearch->setCheckable(true);
    QObject::connect(d->panelsSearch, SIGNAL(triggered()), host, SLOT(slotPanelMenuActionClicked()));

    d->aboutHomepage = new QAction("", host);
    QObject::connect(d->aboutHomepage, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutBuilds = new QAction("", host);
    QObject::connect(d->aboutBuilds, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutSource = new QAction("", host);
    QObject::connect(d->aboutSource, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutIssues = new QAction("", host);
    QObject::connect(d->aboutIssues, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutWiki = new QAction("", host);
    QObject::connect(d->aboutWiki, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutChangelog = new QAction("", host);
    QObject::connect(d->aboutChangelog, SIGNAL(triggered()), host, SLOT(slotAboutOpenUrl()));

    d->aboutClient = new QAction("", host);
    d->aboutClient->setMenuRole(QAction::AboutRole);
    WulforUtil::bindActionIcon(d->aboutClient, AppIcons::eiICON_APPL);
    QObject::connect(d->aboutClient, SIGNAL(triggered()), host, SLOT(slotAboutClient()));

    d->aboutQt = new QAction("", host);
    d->aboutQt->setMenuRole(QAction::AboutQtRole);
    WulforUtil::bindActionIcon(d->aboutQt, AppIcons::eiQT_LOGO);
    QObject::connect(d->aboutQt, SIGNAL(triggered()), host, SLOT(slotAboutQt()));
}
