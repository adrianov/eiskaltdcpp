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
#include "AntiSpamFrame.h"
#include "HashProgress.h"
#include "PmSpamFilter.h"
#include "ShortcutManager.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#include "mainwindow/actions/MenuLabels.h"
#include "mainwindow/actions/WindowActions.h"
#include "mainwindow/WindowChrome.h"
#include "mainwindow/WindowSetup.h"

#include <QCloseEvent>
#include <QEvent>
#include <QHideEvent>
#include <QShowEvent>

using namespace dcpp;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);

    if (WBGET(WB_ANTISPAM_ENABLED)) {
        AntiSpam::newInstance();
        AntiSpam::getInstance()->loadLists();
        AntiSpam::getInstance()->loadSettings();
    }
    PmSpamFilter::newInstance();
    ShortcutManager::newInstance();

    init();
    retranslateUi();
    d->presence.boot(this, d);
}

HashProgress *MainWindow::progress_dialog()
{
    Q_D(MainWindow);
    if (!d->_progress_dialog)
        d->_progress_dialog = new HashProgress(this);
    return d->_progress_dialog;
}

MainWindow::~MainWindow()
{
    Q_D(MainWindow);
    d->presence.teardown(this, d);
    delete d_ptr;
}

void MainWindow::setUnload(bool b)
{
    Q_D(MainWindow);
    d->life.setUnload(b);
}

void MainWindow::closeEvent(QCloseEvent *c_e)
{
    Q_D(MainWindow);
    d->life.onClose(this, d, c_e);
}

void MainWindow::beginExit()
{
    Q_D(MainWindow);
    d->life.beginExit();
}

void MainWindow::show()
{
    Q_D(MainWindow);
    d->place.applyShow(this);
}

void MainWindow::showEvent(QShowEvent *e)
{
    Q_D(MainWindow);
    d->presence.onShow(this, d, e);
}

void MainWindow::getWindowGeometry()
{
    Q_D(MainWindow);
    d->place.capture(this);
}

void MainWindow::hideEvent(QHideEvent *e)
{
    Q_D(MainWindow);
    d->presence.onHide(this, d, e);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ThemeChange)
        reloadSomeSettings();
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    Q_D(MainWindow);
    if (d->presence.filter(this, d, obj, e))
        return true;
    return QMainWindow::eventFilter(obj, e);
}

void MainWindow::init()
{
    Q_D(MainWindow);
    WindowSetup(this, d).run();
}

void MainWindow::loadSettings()
{
    Q_D(MainWindow);
    d->place.load(this, d);
}

void MainWindow::saveSettings()
{
    Q_D(MainWindow);
    d->place.save(this, d);
}

void MainWindow::initActions()
{
    Q_D(MainWindow);
    WindowActions(this, d).build();
}

void MainWindow::initMenuBar()
{
    Q_D(MainWindow);
    WindowChrome(this, d).initMenuBar();
}

void MainWindow::initSearchBar()
{
    Q_D(MainWindow);
    WindowChrome(this, d).initSearchBar();
}

void MainWindow::retranslateUi()
{
    Q_D(MainWindow);
    MenuLabels(d).retranslate();
}

void MainWindow::initToolbar()
{
    Q_D(MainWindow);
    WindowChrome(this, d).initToolbar();
    initFavHubMenu();
}

void MainWindow::initSideBar()
{
    Q_D(MainWindow);
    WindowChrome(this, d).initSideBar();
}

void MainWindow::on(dcpp::LogManagerListener::Message, time_t t, const std::string &m) noexcept
{
    Q_UNUSED(t)
    emit coreLogMessage(_q(m.c_str()));
}
