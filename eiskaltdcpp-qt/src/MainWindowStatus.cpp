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
#include "Notification.h"
#include "PMWindow.h"
#include "WulforUtil.h"
#include "VersionGlobal.h"

#include <QStatusBar>

void MainWindow::initStatusBar(){
    Q_D(MainWindow);
    d->status.build(statusBar(), this, this);
}

void MainWindow::redrawToolPanel(){
    Q_D(MainWindow);

    ArenaWidget *awgt = nullptr;
    PMWindow *pm = nullptr;
    bool has_unread = false;

    auto end = d->menuWidgetsHash.end();
    for (auto it = d->menuWidgetsHash.begin(); it != end; ++it){
        awgt = it.value();
        if (!awgt)
            continue;

        it.key()->setText(awgt->getArenaShortTitle());
        it.key()->setIcon(awgt->getPixmap());

        pm = qobject_cast<PMWindow *>(awgt->getWidget());
        if (pm && pm->hasNewMessages())
            has_unread = true;

        if (d->arena->widget() && d->arena->widget() == awgt->getWidget())
            setWindowTitle(awgt->getArenaTitle() + " :: " + QString::fromStdString(eiskaltdcppAppNameString));
    }

#if !defined(Q_OS_MAC)
    if (!has_unread)
        Notification::getInstance()->resetTrayIcon();
#else
    if (has_unread)
        qApp->setWindowIcon(WICON(WulforUtil::eiMESSAGE_TRAY_ICON));
    else
        qApp->setWindowIcon(WICON(WulforUtil::eiICON_APPL));
#endif

    emit redrawWidgetPanels();
}

void MainWindow::mapWidgetOnArena(ArenaWidget *awgt){
    Q_D(MainWindow);

    if (!(awgt && awgt->getWidget())){
        d->arena->setWidget(nullptr);
        return;
    }

    if (d->arena->widget() != awgt->getWidget())
        d->arena->setWidget(awgt->getWidget());

    setWindowTitle(awgt->getArenaTitle() + " :: " + QString::fromStdString(eiskaltdcppAppNameString));

    if (awgt->toolButton())
        awgt->toolButton()->setChecked(true);

    ArenaWidget::Role role = awgt->role();

    const bool widgetWithFilter = (
                role == ArenaWidget::CmdDebug ||
                role == ArenaWidget::Hub ||
                role == ArenaWidget::PrivateMessage ||
                role == ArenaWidget::PublicHubs ||
                role == ArenaWidget::Search ||
                role == ArenaWidget::Secretary ||
                role == ArenaWidget::ShareBrowser
                );

    const bool widgetWithCleanup = (
                role == ArenaWidget::CmdDebug ||
                role == ArenaWidget::Hub ||
                role == ArenaWidget::PrivateMessage ||
                role == ArenaWidget::SearchSpy ||
                role == ArenaWidget::Secretary
                );

    d->chatClear->setEnabled(widgetWithCleanup);
    d->findInWidget->setEnabled(widgetWithFilter);
    d->chatDisable->setEnabled(role == ArenaWidget::Hub);

    awgt->requestFocus();
}
