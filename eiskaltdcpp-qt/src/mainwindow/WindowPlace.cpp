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

#include "mainwindow/WindowPlace.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "MultiLineToolBar.h"
#include "ToolBar.h"
#include "WulforSettings.h"

#include <QApplication>
#include <QByteArray>
#include <QPoint>
#include <QSize>

void WindowPlace::load(MainWindow *host, MainWindowPrivate *d)
{
    WulforSettings *WS = WulforSettings::getInstance();

    showMax = WS->getBool(WB_MAINWINDOW_MAXIMIZED);
    w = WS->getInt(WI_MAINWINDOW_WIDTH);
    h = WS->getInt(WI_MAINWINDOW_HEIGHT);
    xPos = WS->getInt(WI_MAINWINDOW_X);
    yPos = WS->getInt(WI_MAINWINDOW_Y);

    const QPoint p(xPos, yPos);
    const QSize sz(w, h);

    if (p.x() >= 0 && p.y() >= 0)
        host->move(p);

    if (sz.width() > 0 && sz.height() > 0)
        host->resize(sz);

    const QString dockState = WSGET(WS_MAINWINDOW_STATE);
    if (!dockState.isEmpty())
        host->restoreState(QByteArray::fromBase64(dockState.toUtf8()));

    d->fBar->setVisible(WBGET(WB_TOOLS_PANEL_VISIBLE));
    d->panelsTools->setChecked(WBGET(WB_TOOLS_PANEL_VISIBLE));

    d->sBar->setVisible(WBGET(WB_SEARCH_PANEL_VISIBLE));
    d->panelsSearch->setChecked(WBGET(WB_SEARCH_PANEL_VISIBLE));

    if (d->sideDock) {
        if (d->sideDock->isFloating() && WBGET(WB_MAINWINDOW_HIDE) && WBGET(WB_TRAY_ENABLED))
            d->sideDock->hide();
        else
            d->sideDock->setVisible(WBGET(WB_WIDGETS_PANEL_VISIBLE));
    } else if (MultiLineToolBar *mBar = host->findChild<MultiLineToolBar *>("multiLineTabbar")) {
        mBar->setVisible(WBGET(WB_WIDGETS_PANEL_VISIBLE));
    } else if (ToolBar *tBar = host->findChild<ToolBar *>("tBar")) {
        tBar->setVisible(WBGET(WB_WIDGETS_PANEL_VISIBLE));
    }

    d->panelsWidgets->setChecked(WBGET(WB_WIDGETS_PANEL_VISIBLE));

    if (!WBGET(WB_MAIN_MENU_VISIBLE))
        host->toggleMainMenu(false);

    if (WBGET("mainwindow/dont-show-icons-in-menus", false))
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus);
}

void WindowPlace::save(MainWindow *host, MainWindowPrivate *d)
{
    Q_UNUSED(d)
    static bool stateIsSaved = false;
    if (stateIsSaved)
        return;

    if (host->isVisible())
        showMax = host->isMaximized();

    WBSET(WB_MAINWINDOW_MAXIMIZED, showMax);

    if (!showMax && host->height() > 0 && host->width() > 0) {
        WISET(WI_MAINWINDOW_HEIGHT, host->height());
        WISET(WI_MAINWINDOW_WIDTH, host->width());
    } else {
        WISET(WI_MAINWINDOW_HEIGHT, h);
        WISET(WI_MAINWINDOW_WIDTH, w);
    }

    if (!showMax && host->x() >= 0 && host->y() >= 0) {
        WISET(WI_MAINWINDOW_X, host->x());
        WISET(WI_MAINWINDOW_Y, host->y());
    } else {
        WISET(WI_MAINWINDOW_X, xPos);
        WISET(WI_MAINWINDOW_Y, yPos);
    }

    if (WBGET(WB_MAINWINDOW_REMEMBER))
        WBSET(WB_MAINWINDOW_HIDE, !host->isVisible());

    WSSET(WS_MAINWINDOW_STATE, QString::fromUtf8(host->saveState().toBase64()));
    stateIsSaved = true;
}

void WindowPlace::capture(MainWindow *host)
{
    showMax = host->isMaximized();
    if (!showMax) {
        h = host->height();
        w = host->width();
        xPos = host->x();
        yPos = host->y();
    }
}

void WindowPlace::applyShow(MainWindow *host)
{
    if (showMax)
        host->showMaximized();
    else
        host->showNormal();
}
