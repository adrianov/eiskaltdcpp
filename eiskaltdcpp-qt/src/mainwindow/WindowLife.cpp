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
#include "Notification.h"
#include "TransferView.h"
#include "WulforSettings.h"

#include "dcpp/ProcessExit.h"

#include <QCloseEvent>
#include <QDate>
#include <QMessageBox>

namespace {

bool hideInsteadOfQuit(bool unload)
{
#if defined(Q_OS_MAC)
    return !unload;
#else
    return !unload && WBGET(WB_TRAY_ENABLED);
#endif
}

bool confirmExit(MainWindow *host)
{
    QString msg = MainWindow::tr("Exit program?");
    if (QDate::currentDate().day() == 1 && QDate::currentDate().month() == 4)
        msg = MainWindow::tr("Kill all humans?");
    return QMessageBox::question(host, MainWindow::tr("Action confirm"), msg,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::Yes) == QMessageBox::Yes;
}

void clearExitHistories()
{
    if (WBGET("app/clear-search-history-on-exit", false))
        WSSET(WS_SEARCH_HISTORY, "");
    if (WBGET("app/clear-download-directories-history-on-exit", false))
        WSSET(WS_DOWNLOAD_DIR_HISTORY, "");
}

} // namespace

void WindowLife::beginExit()
{
    exitBegin = true;
    isUnload = true;
    dcpp::noteAppExiting();
}

void WindowLife::onClose(MainWindow *host, MainWindowPrivate *d, QCloseEvent *e)
{
    if (hideInsteadOfQuit(isUnload)) {
        host->hide();
        e->ignore();
        return;
    }

    if (isUnload && WBGET(WB_EXIT_CONFIRM) && !exitBegin) {
        if (!confirmExit(host)) {
            setUnload(false);
            e->ignore();
            return;
        }
        exitBegin = true;
    }

    dcpp::noteAppExiting();
    host->saveSettings();
    clearExitHistories();

    if (d->sideDock)
        d->sideDock->hide();
    d->transfer_dock->hide();
    host->blockSignals(true);

    if (TransferView::getInstance()) {
        TransferView::getInstance()->close();
        TransferView::deleteInstance();
    }
    // Socket joins run after app.exec() so close does not beachball the UI.
    if (Notification::getInstance())
        Notification::deleteInstance();

    d->arena->hide();
    d->arena->setWidget(nullptr);
    e->accept();
}
