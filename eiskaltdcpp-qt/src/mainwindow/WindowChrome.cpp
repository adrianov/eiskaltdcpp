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

#include "mainwindow/WindowChrome.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "ActionCustomizer.h"
#include "SideBar.h"
#include "ToolBar.h"
#include "LineEdit.h"
#include "MultiLineToolBar.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#include "appicon/AppIcons.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QSize>
#include <QToolBar>

namespace {
const QString TOOLBUTTON_STYLE = QStringLiteral("mainwindow/toolbar-toolbutton-style");
const QString SIDEBAR_SHOW_CLOSEBUTTONS = QStringLiteral("mainwindow/sidebar-with-close-buttons");
}

void WindowChrome::initMenuBar()
{
#if defined(Q_OS_MAC)
    host->setMenuBar(new QMenuBar());
    host->menuBar()->setParent(nullptr);
    QObject::connect(host, SIGNAL(destroyed()), host->menuBar(), SLOT(deleteLater()));
#endif

    d->menuFile = new QMenu("", host);
    d->menuFile->addActions(d->fileMenuActions);

    d->menuHubs = new QMenu("", host);
    d->menuHubs->addActions(d->hubsMenuActions);

    d->menuTools = new QMenu("", host);
    d->menuTools->addActions(d->toolsMenuActions);

    QAction *sepPanels = new QAction("", host);
    sepPanels->setSeparator(true);
    d->menuPanels = new QMenu("", host);
    d->menuPanels->addMenu(d->sh_menu);
    d->menuPanels->addAction(sepPanels);
    d->menuPanels->addAction(d->panelsWidgets);
    d->menuPanels->addAction(d->panelsTools);
    d->menuPanels->addAction(d->panelsSearch);

    QAction *sepAbout0 = new QAction("", host);
    sepAbout0->setSeparator(true);
    QAction *sepAbout1 = new QAction("", host);
    sepAbout1->setSeparator(true);
    d->menuAbout = new QMenu("", host);
    d->menuAbout->addAction(d->aboutHomepage);
    d->menuAbout->addAction(d->aboutBuilds);
    d->menuAbout->addAction(d->aboutIssues);
    d->menuAbout->addAction(d->aboutWiki);
    d->menuAbout->addAction(sepAbout0);
    d->menuAbout->addAction(d->aboutChangelog);
    d->menuAbout->addAction(d->aboutSource);
    d->menuAbout->addAction(sepAbout1);
    d->menuAbout->addAction(d->aboutClient);
    d->menuAbout->addAction(d->aboutQt);

    host->menuBar()->addMenu(d->menuFile);
    host->menuBar()->addMenu(d->menuHubs);
    host->menuBar()->addMenu(d->menuTools);
    host->menuBar()->addMenu(d->menuWidgets);
    host->menuBar()->addMenu(d->menuPanels);
    host->menuBar()->addMenu(d->menuAbout);
    host->menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
}

void WindowChrome::initSearchBar()
{
    d->searchLineEdit = new LineEdit(host);
    QObject::connect(d->searchLineEdit, SIGNAL(returnPressed()), host, SLOT(slotToolsSearch()));
}

void WindowChrome::initToolbar()
{
    d->fBar = new ToolBar(host);
    d->fBar->setObjectName("fBar");

    const QStringList enabled_actions =
            QString(QByteArray::fromBase64(WSGET(WS_MAINWINDOW_TOOLBAR_ACTS).toUtf8()))
                    .split(";", WULFOR_SKIP_EMPTY);

    if (enabled_actions.isEmpty()) {
        d->fBar->addActions(d->toolBarActions);
    } else {
        for (const auto &objName : enabled_actions) {
            if (QAction *act = host->findChild<QAction *>(objName))
                d->fBar->addAction(act);
        }
    }

    d->fBar->setContextMenuPolicy(Qt::CustomContextMenu);
    d->fBar->setMovable(true);
    d->fBar->setFloatable(true);
    d->fBar->setAllowedAreas(Qt::AllToolBarAreas);
    d->fBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Actions"));
    d->fBar->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(
            WIGET(TOOLBUTTON_STYLE, Qt::ToolButtonIconOnly)));
    d->fBar->setIconSize(QSize(THEME_ICON_SIZE, THEME_ICON_SIZE));

    QObject::connect(d->fBar, SIGNAL(customContextMenuRequested(QPoint)),
                     host, SLOT(slotToolbarCustomization()));
    host->addToolBar(d->fBar);

    if (!WBGET(WB_MAINWINDOW_USE_SIDEBAR) && WBGET(WB_MAINWINDOW_USE_M_TABBAR)) {
        MultiLineToolBar *mBar = new MultiLineToolBar(host);
        mBar->setContextMenuPolicy(Qt::CustomContextMenu);
        mBar->setVisible(WBGET(WB_WIDGETS_PANEL_VISIBLE));
        QObject::connect(d->nextTabShortCut, SIGNAL(triggered()), mBar, SIGNAL(nextTab()));
        QObject::connect(d->prevTabShortCut, SIGNAL(triggered()), mBar, SIGNAL(prevTab()));
        host->addToolBar(mBar);
    } else if (!WBGET(WB_MAINWINDOW_USE_SIDEBAR) && !WBGET(WB_MAINWINDOW_USE_M_TABBAR)) {
        ToolBar *tBar = new ToolBar(host);
        tBar->setObjectName("tBar");
        tBar->initTabs();
        tBar->setMovable(true);
        tBar->setFloatable(true);
        tBar->setAllowedAreas(Qt::AllToolBarAreas);
        tBar->setContextMenuPolicy(Qt::CustomContextMenu);
        host->addToolBar(tBar);
        QObject::connect(d->nextTabShortCut, SIGNAL(triggered()), tBar, SLOT(nextTab()));
        QObject::connect(d->prevTabShortCut, SIGNAL(triggered()), tBar, SLOT(prevTab()));
    }

    d->sBar = new ToolBar(host);
    d->sBar->setObjectName("sBar");
    d->sBar->addWidget(d->searchLineEdit);
    d->sBar->setContextMenuPolicy(Qt::CustomContextMenu);
    d->sBar->setMovable(true);
    d->sBar->setFloatable(true);
    d->sBar->setAllowedAreas(Qt::AllToolBarAreas);
    d->sBar->setIconSize(QSize(THEME_ICON_SIZE, THEME_ICON_SIZE));
    host->addToolBar(d->sBar);
}

void WindowChrome::initSideBar()
{
    if (!WBGET(WB_MAINWINDOW_USE_SIDEBAR))
        return;

    d->sideDock = new QDockWidget("", host);
    d->sideDock->setWidget(new SideBarView(host));
    d->sideDock->setFeatures(d->sideDock->features() & (~QDockWidget::DockWidgetClosable));
    d->sideDock->setObjectName("sideDock");
    d->sideDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    d->sideDock->setContextMenuPolicy(Qt::CustomContextMenu);
    host->addDockWidget(Qt::LeftDockWidgetArea, d->sideDock);
    QObject::connect(d->sideDock, SIGNAL(customContextMenuRequested(QPoint)),
                     host, SLOT(slotSideBarDockMenu()));
}

void WindowChrome::addToolAction(QAction *act)
{
    if (!d->fBar || d->toolBarActions.contains(act))
        return;

    d->fBar->insertAction(d->toolBarActions.last(), act);
    d->toolBarActions.append(act);
}

void WindowChrome::remToolAction(QAction *act)
{
    if (!d->fBar || !d->toolBarActions.contains(act))
        return;

    d->fBar->removeAction(act);
    d->toolBarActions.removeAt(d->toolBarActions.indexOf(act));
}

void WindowChrome::toggleMainMenu(bool showMenu)
{
    static QAction *compactMenus = nullptr;

    host->menuBar()->setVisible(showMenu);

    if (showMenu) {
        if (compactMenus && d->fBar)
            d->fBar->removeAction(compactMenus);
    } else if (d->fBar) {
        if (!compactMenus) {
            compactMenus = new QAction(MainWindow::tr("Menu"), host);
            compactMenus->setObjectName("compactMenus");
            compactMenus->setIcon(WICON(AppIcons::eiEDIT));
        } else {
            compactMenus->menu()->deleteLater();
            compactMenus->setMenu(nullptr);
        }

        QMenu *m = new QMenu(host);
        for (const auto &a : host->menuBar()->actions())
            m->addAction(a);

        compactMenus->setMenu(m);
        QObject::connect(compactMenus, SIGNAL(triggered()), host, SLOT(slotShowMainMenu()));
        d->fBar->insertAction(d->toolBarActions.first(), compactMenus);
    }

    WBSET(WB_MAIN_MENU_VISIBLE, showMenu);
}

void WindowChrome::hideMainMenu()
{
    toggleMainMenu(!host->menuBar()->isVisible());
}

void WindowChrome::showCompactMenu()
{
    QAction *act = qobject_cast<QAction*>(host->sender());
    if (!(act && act->menu()))
        return;

    act->menu()->exec(QCursor::pos());
}

void WindowChrome::panelMenuClicked()
{
    QAction *act = qobject_cast<QAction *>(host->sender());
    if (!act)
        return;

    if (act == d->panelsWidgets) {
        if (host->findChild<MultiLineToolBar*>("multiLineTabbar"))
            host->findChild<MultiLineToolBar*>("multiLineTabbar")->setVisible(d->panelsWidgets->isChecked());
        else if (host->findChild<ToolBar*>("tBar"))
            host->findChild<ToolBar*>("tBar")->setVisible(d->panelsWidgets->isChecked());
        else if (d->sideDock)
            d->sideDock->setVisible(d->panelsWidgets->isChecked());

        WBSET(WB_WIDGETS_PANEL_VISIBLE, d->panelsWidgets->isChecked());
    } else if (act == d->panelsTools) {
        d->fBar->setVisible(d->panelsTools->isChecked());
        WBSET(WB_TOOLS_PANEL_VISIBLE, d->panelsTools->isChecked());
    } else if (act == d->panelsSearch) {
        d->sBar->setVisible(d->panelsSearch->isChecked());
        WBSET(WB_SEARCH_PANEL_VISIBLE, d->panelsSearch->isChecked());
    }
}

void WindowChrome::customizeToolbar()
{
    QMenu *m = new QMenu(host);
    QMenu *toolButtonStyle = new QMenu(MainWindow::tr("Button style"), host);
    toolButtonStyle->addAction(MainWindow::tr("Icons only"))->setData(Qt::ToolButtonIconOnly);
    toolButtonStyle->addAction(MainWindow::tr("Text only"))->setData(Qt::ToolButtonTextOnly);
    toolButtonStyle->addAction(MainWindow::tr("Text beside icons"))->setData(Qt::ToolButtonTextBesideIcon);
    toolButtonStyle->addAction(MainWindow::tr("Text under icons"))->setData(Qt::ToolButtonTextUnderIcon);

    for (const auto &a : toolButtonStyle->actions()) {
        a->setCheckable(true);
        a->setChecked(d->fBar->toolButtonStyle() == static_cast<Qt::ToolButtonStyle>(a->data().toInt()));
    }

    m->addMenu(toolButtonStyle);
    m->addSeparator();

    QAction *customize = m->addAction(MainWindow::tr("Customize"));
    QAction *ret = m->exec(QCursor::pos());

    m->deleteLater();
    toolButtonStyle->deleteLater();

    if (ret == customize) {
        ActionCustomizer customizer(d->toolBarActions, d->fBar->actions(), host);
        QObject::connect(&customizer, SIGNAL(done(QList<QAction*>)),
                         host, SLOT(slotToolbarCustomizerDone(QList<QAction*>)));
        customizer.exec();
    } else if (ret) {
        d->fBar->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(ret->data().toInt()));
        WISET(TOOLBUTTON_STYLE, static_cast<int>(d->fBar->toolButtonStyle()));
    }
}

void WindowChrome::applyToolbarActions(const QList<QAction*> &enabled)
{
    d->fBar->clear();

    QStringList enabled_list;
    for (const auto &act : enabled) {
        if (!act)
            continue;
        d->fBar->addAction(act);
        enabled_list.push_back(act->objectName());
    }

    host->initFavHubMenu();
    WSSET(WS_MAINWINDOW_TOOLBAR_ACTS, enabled_list.join(";").toUtf8().toBase64());
}

void WindowChrome::sideBarDockMenu()
{
    QMenu *m = new QMenu(host);
    QAction *act = new QAction(MainWindow::tr("Show close buttons"), m);
    act->setCheckable(true);
    act->setChecked(WBGET(SIDEBAR_SHOW_CLOSEBUTTONS, true));
    m->addAction(act);

    if (m->exec(QCursor::pos())) {
        WBSET(SIDEBAR_SHOW_CLOSEBUTTONS, act->isChecked());
        d->sideDock->resize(d->sideDock->size() + QSize(0, -2));
    }

    m->deleteLater();
}

void MainWindow::addActionOnToolBar(QAction *new_act)
{
    WindowChrome(this, d_func()).addToolAction(new_act);
}

void MainWindow::remActionFromToolBar(QAction *act)
{
    WindowChrome(this, d_func()).remToolAction(act);
}

void MainWindow::toggleMainMenu(bool showMenu)
{
    WindowChrome(this, d_func()).toggleMainMenu(showMenu);
}

void MainWindow::slotHideMainMenu()
{
    WindowChrome(this, d_func()).hideMainMenu();
}

void MainWindow::slotShowMainMenu()
{
    WindowChrome(this, d_func()).showCompactMenu();
}

void MainWindow::slotPanelMenuActionClicked()
{
    WindowChrome(this, d_func()).panelMenuClicked();
}

void MainWindow::slotToolbarCustomization()
{
    WindowChrome(this, d_func()).customizeToolbar();
}

void MainWindow::slotToolbarCustomizerDone(const QList<QAction*> &enabled)
{
    WindowChrome(this, d_func()).applyToolbarActions(enabled);
}

void MainWindow::slotSideBarDockMenu()
{
    WindowChrome(this, d_func()).sideBarDockMenu();
}
