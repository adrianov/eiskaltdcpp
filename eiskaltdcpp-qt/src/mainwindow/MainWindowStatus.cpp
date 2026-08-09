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
#include "Notification.h"
#include "PMWindow.h"
#include "WulforUtil.h"
#include "VersionGlobal.h"

#include "dcpp/TimerManager.h"

#include <QAction>
#include <QMenu>
#include <QStatusBar>

void MainWindow::initStatusBar(){
    Q_D(MainWindow);
    d->status.build(statusBar(), this, this);
}

void MainWindow::updateStatus(const QMap<QString, QString> &map){
    Q_D(MainWindow);
    d->status.apply(map, Notification::getInstance(), d->toolsAwayOn, d->toolsAwayOff);
    updateHashProgressStatus();
}

void MainWindow::updateHashProgressStatus() {
    Q_D(MainWindow);
    d->status.updateHashing(d->fileRefreshShareHashProgress, progress_dialog());
}

void MainWindow::setStatusMessage(QString msg){
    Q_D(MainWindow);
    d->status.setLogMessage(msg);
}

void MainWindow::on(dcpp::TimerManagerListener::Second, uint64_t ticks) noexcept{
    Q_D(MainWindow);
    emit coreUpdateStats(d->status.sample(ticks));
}

void MainWindow::slotHideProgressSpace() {
    Q_D(MainWindow);
    d->status.toggleFreeSpace(d->toolsHideProgressSpace);
}

void MainWindow::slotHideLastStatus(){
    Q_D(MainWindow);
    d->status.toggleLastStatus(d->toolsHideLastStatus);
    reloadSomeSettings();
}

void MainWindow::slotHideUsersStatistics(){
    Q_D(MainWindow);
    d->status.toggleUsersStats(d->toolsHideUsersStatisctics);
    reloadSomeSettings();
}

void MainWindow::slotShowSpeedLimits(){
    if (Notification *N = Notification::getInstance())
        N->slotShowSpeedLimits();
}

void MainWindow::slotSuppressTxt(){
    Notification *N = Notification::getInstance();
    QAction *act = qobject_cast<QAction*>(sender());
    if (N && act)
        N->setSuppressTxt(act->isChecked());
}

void MainWindow::slotSuppressSnd(){
    Notification *N = Notification::getInstance();
    QAction *act = qobject_cast<QAction*>(sender());
    if (N && act)
        N->setSuppressSnd(act->isChecked());
}

#if defined(Q_OS_MAC)
void MainWindow::initDockMenuBar(){
    QMenu *menu = new QMenu(this);
    QAction *setup_speed_lim = new QAction(tr("Setup speed limits"), menu);
    setup_speed_lim->setIcon(WICON(AppIcons::eiSPEED_LIMIT_ON));

    QMenu *menuAdditional = new QMenu(tr("Additional"), this);
    QAction *actSuppressSnd = new QAction(tr("Suppress sound notifications"), menuAdditional);
    QAction *actSuppressTxt = new QAction(tr("Suppress text notifications"), menuAdditional);
    actSuppressSnd->setCheckable(true);
    actSuppressTxt->setCheckable(true);

    connect(setup_speed_lim, SIGNAL(triggered()), this, SLOT(slotShowSpeedLimits()));
    connect(actSuppressTxt, SIGNAL(triggered()), this, SLOT(slotSuppressTxt()));
    connect(actSuppressSnd, SIGNAL(triggered()), this, SLOT(slotSuppressSnd()));

    menuAdditional->addActions(QList<QAction*>() << actSuppressTxt << actSuppressSnd);
    menu->addAction(setup_speed_lim);
    menu->addMenu(menuAdditional);
    menu->setAsDockMenu();
}
#endif

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
        qApp->setWindowIcon(WICON(AppIcons::eiMESSAGE_TRAY_ICON));
    else
        qApp->setWindowIcon(WICON(AppIcons::eiICON_APPL));
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
