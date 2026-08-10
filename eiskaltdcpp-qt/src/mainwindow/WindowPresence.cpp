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

#include "mainwindow/WindowPresence.h"
#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "AntiSpamFrame.h"
#include "AppTheme.h"
#include "PmSpamFilter.h"
#include "SearchBlacklist.h"
#include "ShortcutManager.h"
#include "TransferView.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/LogManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/TimerManager.h"
#include "dcpp/Util.h"

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QHideEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QShowEvent>
#include <QSize>

using namespace dcpp;

namespace {

bool roleHasFilter(ArenaWidget::Role role)
{
    switch (role) {
    case ArenaWidget::Hub:
    case ArenaWidget::PrivateMessage:
    case ArenaWidget::ShareBrowser:
    case ArenaWidget::PublicHubs:
    case ArenaWidget::Search:
        return true;
    default:
        return false;
    }
}

void syncArenaActions(MainWindowPrivate *d, ArenaWidget *awgt)
{
    const ArenaWidget::Role role = awgt->role();
    const bool chat = role == ArenaWidget::Hub || role == ArenaWidget::PrivateMessage;
    d->chatClear->setEnabled(chat);
    d->findInWidget->setEnabled(roleHasFilter(role));
    d->chatDisable->setEnabled(role == ArenaWidget::Hub);
}

} // namespace

void WindowPresence::ensureNick(MainWindow *host)
{
    if (!_q(SETTING(NICK)).isEmpty())
        return;

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

void WindowPresence::boot(MainWindow *host, MainWindowPrivate *d)
{
    LogManager::getInstance()->addListener(host);
    TimerManager::getInstance()->addListener(host);
    // Listen sockets open later (finishBoot), after paint and share load.
    host->setStatusMessage(MainWindow::tr("Starting..."));

    TransferView::newInstance();
    d->transfer_dock->setWidget(TransferView::getInstance());
    d->toolsTransfers->setChecked(d->transfer_dock->isVisible());

    if (!WSGET(WS_APP_THEME).isEmpty())
        qApp->setStyle(WSGET(WS_APP_THEME));
    else
        AppTheme::applyPreferredStyle();
}

void WindowPresence::finishBoot(MainWindow *host, MainWindowPrivate *d)
{
    Q_UNUSED(d);
    host->startSocket(false);
    host->setStatusMessage(MainWindow::tr("Ready"));

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

void WindowPresence::teardown(MainWindow *host, MainWindowPrivate *d)
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

void WindowPresence::onShow(MainWindow *host, MainWindowPrivate *d, QShowEvent *e)
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

    if (ArenaWidget *awgt = qobject_cast<ArenaWidget *>(d->arena->widget()))
        syncArenaActions(d, awgt);

    ensureNick(host);
    e->accept();
}

void WindowPresence::onHide(MainWindow *host, MainWindowPrivate *d, QHideEvent *e)
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

bool WindowPresence::filter(MainWindow *host, MainWindowPrivate *d, QObject *obj, QEvent *e)
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
