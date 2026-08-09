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

#include "mainwindow/WindowLife.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "AntiSpamFrame.h"
#include "AppTheme.h"
#include "Notification.h"
#include "PmSpamFilter.h"
#include "SearchBlacklist.h"
#include "ShortcutManager.h"
#include "TransferView.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/ConnectionManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/TimerManager.h"
#include "dcpp/Util.h"

#include <QApplication>
#include <QDir>

#include <QCloseEvent>
#include <QDate>
#include <QEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QShowEvent>
#include <QHideEvent>
#include <QSize>

using namespace dcpp;

void WindowLife::beginExit()
{
    exitBegin = true;
    isUnload = true;
}

void WindowLife::boot(MainWindow *host, MainWindowPrivate *d)
{
    LogManager::getInstance()->addListener(host);
    TimerManager::getInstance()->addListener(host);
    host->startSocket(false);
    host->setStatusMessage(MainWindow::tr("Ready"));

    TransferView::newInstance();
    d->transfer_dock->setWidget(TransferView::getInstance());
    d->toolsTransfers->setChecked(d->transfer_dock->isVisible());

    if (!WSGET(WS_APP_THEME).isEmpty())
        qApp->setStyle(WSGET(WS_APP_THEME));
    else
        AppTheme::applyPreferredStyle();

    if (!WBGET(WB_APP_REMOVE_NOT_EX_DIRS))
        return;

    const StringPairList directories = ShareManager::getInstance()->getDirectories();
    for (const auto &it : directories) {
        if (QDir(_q(it.second)).exists())
            continue;
        try {
            ShareManager::getInstance()->removeDirectory(it.second);
        } catch (const std::exception &) {
        }
    }
}

void WindowLife::teardown(MainWindow *host, MainWindowPrivate *d)
{
    LogManager::getInstance()->removeListener(host);
    TimerManager::getInstance()->removeListener(host);

    if (AntiSpam::getInstance()) {
        AntiSpam::getInstance()->saveLists();
        AntiSpam::getInstance()->saveSettings();
        AntiSpam::deleteInstance();
    }

    delete d->arena;
    delete d->fBar;
    delete d->sBar;

    ShortcutManager::deleteInstance();
    SearchBlacklist::deleteInstance();
    PmSpamFilter::deleteInstance();
}

void WindowLife::onClose(MainWindow *host, MainWindowPrivate *d, QCloseEvent *e)
{
#if defined(Q_OS_MAC)
    if (!isUnload) {
#else
    if (!isUnload && WBGET(WB_TRAY_ENABLED)) {
#endif
        host->hide();
        e->ignore();
        return;
    }

    if (isUnload && WBGET(WB_EXIT_CONFIRM) && !exitBegin) {
        QString msg = MainWindow::tr("Exit program?");
        if (QDate::currentDate().day() == 1 && QDate::currentDate().month() == 4)
            msg = MainWindow::tr("Kill all humans?");
        if (QMessageBox::question(host, MainWindow::tr("Action confirm"), msg,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) != QMessageBox::Yes) {
            setUnload(false);
            e->ignore();
            return;
        }
        exitBegin = true;
    }

    host->saveSettings();

    if (WBGET("app/clear-search-history-on-exit", false))
        WSSET(WS_SEARCH_HISTORY, "");
    if (WBGET("app/clear-download-directories-history-on-exit", false))
        WSSET(WS_DOWNLOAD_DIR_HISTORY, "");

    if (d->sideDock)
        d->sideDock->hide();
    d->transfer_dock->hide();
    host->blockSignals(true);

    if (TransferView::getInstance()) {
        TransferView::getInstance()->close();
        TransferView::deleteInstance();
    }
    if (SearchManager::getInstance())
        SearchManager::getInstance()->disconnect();
    if (ConnectionManager::getInstance())
        ConnectionManager::getInstance()->disconnect();
    if (Notification::getInstance())
        Notification::deleteInstance();

    d->arena->hide();
    d->arena->setWidget(nullptr);
    e->accept();
}

void WindowLife::onShow(MainWindow *host, MainWindowPrivate *d, QShowEvent *e)
{
    if (!d->place.showMax && d->place.w > 0 && d->place.h > 0
            && d->place.w != host->width() && d->place.h != host->height())
        host->resize(QSize(d->place.w, d->place.h));

    if (WBGET(WB_APP_AUTO_AWAY) && !Util::getManualAway()) {
        Util::setAway(false);
        d->toolsAwayOff->setChecked(true);
    }

    if (d->transfer_dock->isVisible())
        d->toolsTransfers->setChecked(true);
    if (d->sideDock)
        d->sideDock->setVisible(d->panelsWidgets->isChecked());

    ArenaWidget *awgt = qobject_cast<ArenaWidget *>(d->arena->widget());
    if (!awgt) {
        e->accept();
        return;
    }

    const ArenaWidget::Role role = awgt->role();
    const bool withFilter = role == ArenaWidget::Hub
            || role == ArenaWidget::PrivateMessage
            || role == ArenaWidget::ShareBrowser
            || role == ArenaWidget::PublicHubs
            || role == ArenaWidget::Search;
    d->chatClear->setEnabled(role == ArenaWidget::Hub || role == ArenaWidget::PrivateMessage);
    d->findInWidget->setEnabled(withFilter);
    d->chatDisable->setEnabled(role == ArenaWidget::Hub);

    if (_q(SETTING(NICK)).isEmpty()) {
        host->activateWindow();
        host->raise();
        bool ok = false;
        const QString nick = QInputDialog::getText(
                host, MainWindow::tr("Enter user nick"), MainWindow::tr("Nick"),
                QLineEdit::Normal, MainWindow::tr("User"), &ok);
        if (ok && !nick.isEmpty()) {
            SettingsManager::getInstance()->set(SettingsManager::NICK, _tq(nick));
            ok = (QMessageBox::question(
                          host, "EiskaltDC++",
                          MainWindow::tr("Would you like to change other settings?"),
                          QMessageBox::Yes, QMessageBox::No)
                  == QMessageBox::No);
        }
        if (!ok)
            host->slotToolsSettings();
    }
    e->accept();
}

void WindowLife::onHide(MainWindow *host, MainWindowPrivate *d, QHideEvent *e)
{
    d->place.capture(host);
    e->accept();
    if (d->sideDock && d->sideDock->isFloating())
        d->sideDock->hide();
    if (WBGET(WB_APP_AUTO_AWAY)) {
        Util::setAway(true);
        d->toolsAwayOn->setChecked(true);
    }
}

bool WindowLife::filter(MainWindow *host, MainWindowPrivate *d, QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::WindowActivate) {
        host->redrawToolPanel();
    } else if (obj == d->status.hashWidget() && e->type() == QEvent::MouseButtonDblClick) {
        host->slotFileHashProgress();
        return true;
    } else if (obj == d->status.freeSpaceWidget() && e->type() == QEvent::MouseButtonDblClick) {
        host->slotFileOpenDownloadDirectory();
        return true;
    }
    return false;
}
