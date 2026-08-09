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

#include "ArenaWidgetManager.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "Notification.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/ConnectivityManager.h"

#include <QAction>
#include <QMessageBox>

using namespace dcpp;

void MainWindow::startSocket(bool changed)
{
    if (changed)
        ConnectivityManager::getInstance()->updateLast();
    try {
        ConnectivityManager::getInstance()->setup(true);
    } catch (const Exception& e) {
        showPortsError(e.getError());
    }
    ClientManager::getInstance()->infoUpdated();
}

void MainWindow::showPortsError(const string& port)
{
    QString msg = tr("Unable to open %1 port. Searching or file transfers will not work correctly until you change settings or turn off any application that might be using that port.").arg(_q(port));
    QMessageBox::warning(this, tr("Connectivity Manager: Warning"), msg, QMessageBox::Ok);
}

void MainWindow::slotShareIndexQueueEmpty()
{
    emit notifyMessage(Notification::TRANSFER, tr("Download Queue"), tr("All downloads complete"));
}

void MainWindow::slotHubsReconnect()
{
    HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->activeHub());
    if (fr)
        fr->reconnect();
}

void MainWindow::updateActionIcons()
{
    WulforUtil *WU = WulforUtil::getInstance();

    for (QAction *act : findChildren<QAction*>()) {
        const QVariant iconId = act->property("wulforIcon");
        if (!iconId.isValid())
            continue;
        act->setIcon(WU->getIcon(static_cast<WulforUtil::Icons>(iconId.toInt())));
    }
}

void MainWindow::slotChatClear()
{
    Q_D(MainWindow);

    if (!d->arena->widget() || !qobject_cast<ArenaWidget*>(d->arena->widget()))
        return;

    ArenaWidget *awgt = qobject_cast<ArenaWidget*>(d->arena->widget());
    awgt->requestClear();
}

void MainWindow::slotFind()
{
    Q_D(MainWindow);

    if (!d->arena->widget() || !qobject_cast<ArenaWidget*>(d->arena->widget()))
        return;

    ArenaWidget *awgt = qobject_cast<ArenaWidget*>(d->arena->widget());
    awgt->requestFilter();
}

void MainWindow::slotChatDisable()
{
    HubFrame *fr = qobject_cast<HubFrame*>(HubManager::getInstance()->activeHub());
    if (fr)
        fr->disableChat();
}

void MainWindow::slotWidgetsToggle()
{
    Q_D(MainWindow);

    QAction *act = reinterpret_cast<QAction*>(sender());
    auto it = d->menuWidgetsHash.find(act);
    if (it == d->menuWidgetsHash.end())
        return;

    ArenaWidgetManager::getInstance()->activate(it.value());
}

void MainWindow::slotHideWindow()
{
    Q_D(MainWindow);

    if (!d->life.isUnload && isActiveWindow() && WBGET(WB_TRAY_ENABLED))
        hide();
}

void MainWindow::slotExit()
{
    setUnload(true);
    close();
}

void MainWindow::slotUnixSignal(int sig)
{
    printf("Received unix signal %i\n", sig);
}

void MainWindow::slotCloseCurrentWidget()
{
    Q_D(MainWindow);

    ArenaWidget *awgt = dynamic_cast<ArenaWidget*>(d->arena->widget());
    if (awgt)
        ArenaWidgetManager::getInstance()->rem(awgt);
}
