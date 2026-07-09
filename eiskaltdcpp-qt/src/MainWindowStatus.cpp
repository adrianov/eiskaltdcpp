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
#include "StatusBarLogLabel.h"
#include "PMWindow.h"
#include "WulforUtil.h"
#include "WulforSettings.h"
#include "VersionGlobal.h"

#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>

void MainWindow::initStatusBar(){
    Q_D(MainWindow);

    d->statusLabel = new QLabel(statusBar());
    d->statusLabel->setFrameShadow(QFrame::Sunken);
    d->statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->statusLabel->setToolTip(tr("Counts"));
    d->statusLabel->setContentsMargins(0, 0, 0, 0);

    d->statusSPLabel = new QLabel(statusBar());
    d->statusSPLabel->setFrameShadow(QFrame::Sunken);
    d->statusSPLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->statusSPLabel->setToolTip(tr("Download/Upload speed"));
    d->statusSPLabel->setContentsMargins(0, 0, 0, 0);

    d->statusDLabel = new QLabel(statusBar());
    d->statusDLabel->setFrameShadow(QFrame::Sunken);
    d->statusDLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->statusDLabel->setToolTip(tr("Downloaded/Uploaded"));
    d->statusDLabel->setContentsMargins(0, 0, 0, 0);

    d->msgLabel = new StatusBarLogLabel(statusBar());
    d->msgLabel->setHeightRef(d->statusLabel);

#if (defined FREE_SPACE_BAR_C)
#if defined(USE_PROGRESS_BARS)
    d->progressFreeSpace = new QProgressBar(this);
    d->progressFreeSpace->setMaximum(100);
    d->progressFreeSpace->setMinimum(0);
    d->progressFreeSpace->setAlignment(Qt::AlignHCenter);
#else
    d->progressFreeSpace = new QLabel(this);
    d->progressFreeSpace->setTextFormat(Qt::PlainText);
    d->progressFreeSpace->setText(tr("Space free"));
    d->progressFreeSpace->setAlignment(Qt::AlignRight);
#endif

    d->progressFreeSpace->setMinimumWidth(100);
    d->progressFreeSpace->setMaximumWidth(250);
    d->progressFreeSpace->setFixedHeight(18);
    d->progressFreeSpace->setToolTip(tr("Space free"));
    d->progressFreeSpace->installEventFilter(this);

    if (!WBGET(WB_SHOW_FREE_SPACE))
        d->progressFreeSpace->hide();
#else
    WBSET(WB_SHOW_FREE_SPACE, false);
#endif

    d->progressHashing = new QProgressBar(this);
    d->progressHashing->setMaximum(100);
    d->progressHashing->setMinimum(0);
    d->progressHashing->setAlignment( Qt::AlignHCenter );
    d->progressHashing->setFixedHeight(18);
    d->progressHashing->setToolTip(tr("Hashing progress"));
    d->progressHashing->hide();
    d->progressHashing->installEventFilter( this );

    statusBar()->addWidget(d->progressHashing);
    statusBar()->addWidget(d->msgLabel, 1);
    statusBar()->addPermanentWidget(d->statusDLabel);
    statusBar()->addPermanentWidget(d->statusSPLabel);
    statusBar()->addPermanentWidget(d->statusLabel);
#if (defined FREE_SPACE_BAR_C)
    statusBar()->addPermanentWidget(d->progressFreeSpace);
#endif
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
