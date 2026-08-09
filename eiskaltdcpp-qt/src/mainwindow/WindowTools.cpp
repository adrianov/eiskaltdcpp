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

#include "mainwindow/WindowTools.h"

#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "AntiSpamFrame.h"
#include "AppTheme.h"
#include "ArenaWidgetFactory.h"
#include "ArenaWidgetManager.h"
#include "HubFrame.h"
#include "IPFilterFrame.h"
#include "QuickConnect.h"
#include "SearchFrame.h"
#include "Settings.h"
#include "TransferView.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#include "appicon/AppIcons.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/Util.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMessageBox>
#include <stdexcept>

#ifdef USE_JS
#include "ScriptManagerDialog.h"
#include "scriptengine/ScriptConsole.h"
#include "scriptengine/ScriptEngine.h"
#endif

using namespace dcpp;

namespace {
void bindSpeedLimitIcon(QAction *act, bool enabled)
{
    WulforUtil::bindActionIcon(act, enabled ? AppIcons::eiSPEED_LIMIT_ON : AppIcons::eiSPEED_LIMIT_OFF);
}

#ifdef USE_JS
enum class ScriptChangedAction: int {
    DoNothing = 0,
    AskUser,
    ReloadIt
};
#endif
}

void WindowTools::toggleRole(ArenaWidget::Role role)
{
    toggleSingleton(host->widgetForRole(role));
}

void WindowTools::toggleSingleton(ArenaWidget *a)
{
    if (!a)
        throw std::runtime_error(_tq(Q_FUNC_INFO) + ": NULL argument");

    if (host->sender() && qobject_cast<QAction*>(host->sender()) && a->getWidget()) {
        QAction *act = reinterpret_cast<QAction*>(host->sender());
        act->setCheckable(true);
        a->setToolButton(act);
    }

    if (!a->getWidget()->isVisible())
        ArenaWidgetManager::getInstance()->activate(a);
    else
        ArenaWidgetManager::getInstance()->toggle(a);
}

void WindowTools::openSearch()
{
    SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();
    QLineEdit *le = qobject_cast<QLineEdit *>(host->sender());
    if (le != d->searchLineEdit)
        return;

    QString text = d->searchLineEdit->text();
    bool isTTH = false;

    if (text.startsWith("magnet:")) {
        QString link = text;
        QString tth, name;
        int64_t size = 0;
        WulforUtil::splitMagnet(link, size, tth, name);
        text = tth;
        isTTH = true;
    }

    sf->fastSearch(text, isTTH || WulforUtil::isTTH(text));
}

void WindowTools::openAntiSpam()
{
    AntiSpamFrame fr(host);
    fr.exec();
    d->toolsAntiSpam->setChecked(AntiSpam::getInstance() != nullptr);
}

void WindowTools::openIpFilter()
{
    IPFilterFrame fr(host);
    fr.exec();
    d->toolsIPFilter->setChecked(BOOLSETTING(SettingsManager::IPFILTER));
}

void WindowTools::setAutoAway()
{
    WBSET(WB_APP_AUTO_AWAY, d->toolsAutoAway->isChecked());
}

void WindowTools::switchAway()
{
    if ((host->sender() != d->toolsAwayOff) && (host->sender() != d->toolsAwayOn))
        return;

    bool away = d->toolsAwayOn->isChecked();
    Util::setAway(away);
    Util::setManualAway(away);
}

void WindowTools::openScriptManager()
{
#ifdef USE_JS
    ScriptManagerDialog(host).exec();
#endif
}

void WindowTools::scriptFileChanged(const QString &script)
{
#ifdef USE_JS
    enum ScriptChangedAction act = (enum ScriptChangedAction)WIGET("scriptmanager/script-changed-action", 0);
    bool ask = false;

    switch (act) {
    case ScriptChangedAction::DoNothing:
        break;
    case ScriptChangedAction::AskUser:
        ask = true;
    case ScriptChangedAction::ReloadIt: {
        auto raiseMe = [this]() -> bool {
            if (!host->isVisible()) {
                host->show();
                host->raise();
            }
            return true;
        };

        if (ask && raiseMe() && (QMessageBox::warning(host,
                                                      MainWindow::tr("Script Engine"),
                                                      QString("\'%1\' has been changed. Reload it?").arg(script),
                                                      QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes))
            break;

        ScriptEngine::getInstance()->loadScript(script);
        break;
    }
    }
#else
    Q_UNUSED(script)
#endif
}

void WindowTools::openScriptConsole()
{
#ifdef USE_JS
    if (!d->scriptConsole)
        d->scriptConsole = new ScriptConsole(host);

    d->scriptConsole->setWindowModality(Qt::NonModal);
    d->scriptConsole->show();
    d->scriptConsole->raise();
#endif
}

void WindowTools::openSettings()
{
    Settings s;
    s.exec();
    reloadSettings();

    if (!WBGET(WB_TRAY_ENABLED))
        d->fileHideWindow->setText(MainWindow::tr("Show/hide find frame"));
    else
        d->fileHideWindow->setText(MainWindow::tr("Hide window"));
}

void WindowTools::toggleTransferDock(bool toggled)
{
    if (toggled) {
        d->transfer_dock->setVisible(true);
        d->transfer_dock->setWidget(TransferView::getInstance());
    } else {
        d->transfer_dock->setWidget(nullptr);
        d->transfer_dock->setVisible(false);
    }
}

void WindowTools::switchSpeedLimit()
{
    SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, d->toolsSwitchSpeedLimit->isChecked());
    bindSpeedLimitIcon(d->toolsSwitchSpeedLimit, BOOLSETTING(THROTTLE_ENABLE));
}

void WindowTools::copyWindowTitle()
{
    QString text = host->windowTitle();
    if (!text.isEmpty())
        qApp->clipboard()->setText(text, QClipboard::Clipboard);
}

void WindowTools::quickConnect()
{
    QuickConnect qc;
    qc.exec();
}

void WindowTools::reloadSettings()
{
    AppTheme::apply();

    for (const auto &awgt : d->menuWidgetsHash.values()) {
        HubFrame *fr = qobject_cast<HubFrame *>(awgt->getWidget());
        if (fr)
            fr->reloadSomeSettings();
    }

    d->toolsSwitchSpeedLimit->setChecked(BOOLSETTING(THROTTLE_ENABLE));
}

void MainWindow::slotToolsADLS()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::ADLS);
}

void MainWindow::slotToolsCmdDebug()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::CmdDebug);
}

void MainWindow::slotToolsSecretary()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::Secretary);
}

void MainWindow::slotToolsDownloadQueue()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::Downloads);
}

void MainWindow::slotToolsQueuedUsers()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::QueuedUsers);
}

void MainWindow::slotToolsHubManager()
{
}

void MainWindow::slotToolsFinishedDownloads()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::FinishedDownloads);
}

void MainWindow::slotToolsFinishedUploads()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::FinishedUploads);
}

void MainWindow::slotToolsSpy()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::SearchSpy);
}

void MainWindow::slotHubsFavoriteHubs()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::FavoriteHubs);
}

void MainWindow::slotHubsPublicHubs()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::PublicHubs);
}

void MainWindow::slotHubsFavoriteUsers()
{
    WindowTools(this, d_func()).toggleRole(ArenaWidget::FavoriteUsers);
}

void MainWindow::toggleSingletonWidget(ArenaWidget *a)
{
    WindowTools(this, d_func()).toggleSingleton(a);
}

void MainWindow::slotToolsSearch()
{
    WindowTools(this, d_func()).openSearch();
}

void MainWindow::slotToolsAntiSpam()
{
    WindowTools(this, d_func()).openAntiSpam();
}

void MainWindow::slotToolsIPFilter()
{
    WindowTools(this, d_func()).openIpFilter();
}

void MainWindow::slotToolsAutoAway()
{
    WindowTools(this, d_func()).setAutoAway();
}

void MainWindow::slotToolsSwitchAway()
{
    WindowTools(this, d_func()).switchAway();
}

void MainWindow::slotToolsJS()
{
    WindowTools(this, d_func()).openScriptManager();
}

void MainWindow::slotJSFileChanged(const QString &script)
{
    WindowTools(this, d_func()).scriptFileChanged(script);
}

void MainWindow::slotToolsJSConsole()
{
    WindowTools(this, d_func()).openScriptConsole();
}

void MainWindow::slotToolsCopyWindowTitle()
{
    WindowTools(this, d_func()).copyWindowTitle();
}

void MainWindow::slotToolsSettings()
{
    WindowTools(this, d_func()).openSettings();
}

void MainWindow::slotToolsTransfer(bool toggled)
{
    WindowTools(this, d_func()).toggleTransferDock(toggled);
}

void MainWindow::slotToolsSwitchSpeedLimit()
{
    WindowTools(this, d_func()).switchSpeedLimit();
}

void MainWindow::reloadSomeSettings()
{
    WindowTools(this, d_func()).reloadSettings();
}

void MainWindow::slotQC()
{
    WindowTools(this, d_func()).quickConnect();
}
