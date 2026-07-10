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
#include "ArenaWidgetFactory.h"
#include "SearchFrame.h"
#include "AntiSpamFrame.h"
#include "IPFilterFrame.h"
#include "Settings.h"
#include "TransferView.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/Util.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMessageBox>

#ifdef USE_JS
#include "ScriptManagerDialog.h"
#include "scriptengine/ScriptConsole.h"
#include "scriptengine/ScriptEngine.h"
#endif

using namespace dcpp;

namespace {
void bindSpeedLimitIcon(QAction *act, bool enabled)
{
    WulforUtil::bindActionIcon(act, enabled ? WulforUtil::eiSPEED_LIMIT_ON : WulforUtil::eiSPEED_LIMIT_OFF);
}

#ifdef USE_JS
enum class ScriptChangedAction: int {
    DoNothing=0,
    AskUser,
    ReloadIt
};
#endif
}

void MainWindow::slotToolsADLS(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::ADLS));
}

void MainWindow::slotToolsCmdDebug()
{
    toggleSingletonWidget(widgetForRole(ArenaWidget::CmdDebug));
}

void MainWindow::slotToolsSecretary()
{
    toggleSingletonWidget(widgetForRole(ArenaWidget::Secretary));
}

void MainWindow::slotToolsSearch() {
    SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame>();

    QLineEdit *le = qobject_cast<QLineEdit *> ( sender() );

    Q_D(MainWindow);

    if ( le != d->searchLineEdit )
        return;

    QString text = d->searchLineEdit->text();
    bool isTTH = false;

    if ( text.startsWith ( "magnet:" ) ) {
        QString link = text;
        QString tth = "", name = "";
        int64_t size = 0;

        WulforUtil::splitMagnet ( link, size, tth, name );

        text  = tth;
        isTTH = true;
    }

    sf->fastSearch ( text, isTTH || WulforUtil::isTTH ( text ) );
}

void MainWindow::slotToolsDownloadQueue(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::Downloads));
}

void MainWindow::slotToolsQueuedUsers(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::QueuedUsers));
}

void MainWindow::slotToolsHubManager(){
}

void MainWindow::slotToolsFinishedDownloads(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::FinishedDownloads));
}

void MainWindow::slotToolsFinishedUploads(){
   toggleSingletonWidget(widgetForRole(ArenaWidget::FinishedUploads));
}

void MainWindow::slotToolsSpy(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::SearchSpy));
}

void MainWindow::slotToolsAntiSpam(){
    AntiSpamFrame fr(this);

    fr.exec();

    Q_D(MainWindow);

    d->toolsAntiSpam->setChecked(AntiSpam::getInstance() != nullptr);
}

void MainWindow::slotToolsIPFilter(){
    IPFilterFrame fr(this);

    fr.exec();

    Q_D(MainWindow);

    d->toolsIPFilter->setChecked(BOOLSETTING(SettingsManager::IPFILTER));
}

void MainWindow::slotToolsAutoAway(){
    Q_D(MainWindow);

    WBSET(WB_APP_AUTO_AWAY, d->toolsAutoAway->isChecked());
}

void MainWindow::slotToolsSwitchAway(){
    Q_D(MainWindow);

    if ((sender() != d->toolsAwayOff) && (sender() != d->toolsAwayOn))
        return;

    bool away = d->toolsAwayOn->isChecked();

    Util::setAway(away);
    Util::setManualAway(away);
}

void MainWindow::slotToolsJS(){
#ifdef USE_JS
    ScriptManagerDialog(this).exec();
#endif
}

void MainWindow::slotJSFileChanged(const QString &script){
#ifdef USE_JS
    enum ScriptChangedAction act = (enum ScriptChangedAction)WIGET("scriptmanager/script-changed-action", 0);
    bool ask = false;

    switch (act){
    case ScriptChangedAction::DoNothing:
        break;
    case ScriptChangedAction::AskUser:
        ask = true;
    case ScriptChangedAction::ReloadIt:
    {
        auto raiseMe = [this]() -> bool {
            if (!this->isVisible()){
                this->show();
                this->raise();
            }

            return true;
        };

        if (ask && raiseMe() && (QMessageBox::warning(this,
                                                      tr("Script Engine"),
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


void MainWindow::slotToolsJSConsole(){
#ifdef USE_JS
    Q_D(MainWindow);

    if (!d->scriptConsole)
        d->scriptConsole = new ScriptConsole(this);

    d->scriptConsole->setWindowModality(Qt::NonModal);
    d->scriptConsole->show();
    d->scriptConsole->raise();
#endif
}

void MainWindow::slotHubsFavoriteHubs(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::FavoriteHubs));
}

void MainWindow::slotHubsPublicHubs(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::PublicHubs));
}

void MainWindow::slotHubsFavoriteUsers(){
    toggleSingletonWidget(widgetForRole(ArenaWidget::FavoriteUsers));
}

void MainWindow::slotToolsCopyWindowTitle(){
    QString text = windowTitle();

    if (!text.isEmpty())
        qApp->clipboard()->setText(text, QClipboard::Clipboard);
}

void MainWindow::slotToolsSettings(){
    Settings s;

    s.exec();

    reloadSomeSettings();

    Q_D(MainWindow);

    //reload some settings
    if (!WBGET(WB_TRAY_ENABLED))
        d->fileHideWindow->setText(tr("Show/hide find frame"));
    else
        d->fileHideWindow->setText(tr("Hide window"));
}

void MainWindow::slotToolsTransfer(bool toggled){
    Q_D(MainWindow);

    if (toggled){
        d->transfer_dock->setVisible(true);
        d->transfer_dock->setWidget(TransferView::getInstance());
    }
    else {
        d->transfer_dock->setWidget(nullptr);
        d->transfer_dock->setVisible(false);
    }
}

void MainWindow::slotToolsSwitchSpeedLimit(){
    Q_D(MainWindow);

    SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, d->toolsSwitchSpeedLimit->isChecked());
    bindSpeedLimitIcon(d->toolsSwitchSpeedLimit, BOOLSETTING(THROTTLE_ENABLE));
}
