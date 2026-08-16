/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "appshell/session/SessionBootstrap.h"
#include "appshell/session/ShareIndexLife.h"
#include "appshell/session/UnixSignals.h"

#include "dcpp/stdinc.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/DCPlusPlus.h"
#include "dcpp/HashManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/ProcessExit.h"
#include "dcpp/SearchManager.h"
#include "dcpp/Thread.h"
#include "dcpp/TimerManager.h"
#include "dcpp/Singleton.h"

#include "WulforUtil.h"
#include "WulforSettings.h"
#include "HubManager.h"
#include "Notification.h"
#include "VersionGlobal.h"
#include "Antispam.h"
#include "EmoticonFactory.h"
#include "FinishedTransfers.h"
#include "queuedusers/QueuedUsers.h"
#include "ArenaWidgetManager.h"
#include "ArenaWidgetFactory.h"
#include "MainWindow.h"
#include "GlobalTimer.h"

#if defined(Q_OS_HAIKU)
#include "EiskaltApp_haiku.h"
#elif defined(Q_OS_MAC)
#include "EiskaltApp_mac.h"
#else
#include "EiskaltApp.h"
#endif

#ifdef USE_ASPELL
#include "SpellCheck.h"
#endif

#ifdef USE_JS
#include "ScriptEngine.h"
#endif

#include <QCoreApplication>
#include <QEventLoop>
#include <QObject>
#include <QThread>

#include <atomic>
#include <iostream>
#include <thread>

extern void callBack(void *, const std::string &);

namespace {

void pumpUntil(std::atomic_bool &done)
{
    while (!done.load(std::memory_order_acquire)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(20);
    }
}

} // namespace

SessionBootstrap::SessionBootstrap(EiskaltApp &app)
    : app_(app)
{
}

void SessionBootstrap::bringUp()
{
    startCoreShell();
    startUi();
    startServices();
    showAndPaint();
    loadShareData();
    // Timers after share load so Minute auto-refresh cannot race the first refresh.
    dcpp::TimerManager::getInstance()->start();
    MainWindow::getInstance()->finishStartup();
    startDeferredServices();
    MainWindow::getInstance()->autoconnect();
    MainWindow::getInstance()->parseCmdLine(app_.arguments());
}

void SessionBootstrap::tearDown()
{
    stopUi();
    stopCore();
}

void SessionBootstrap::startCoreShell()
{
    dcpp::startupShell(callBack, nullptr);

    const string prev = dcpp::checkPreviousSession();
    if (!prev.empty())
        dcpp::LogManager::getInstance()->message(prev);
    dcpp::markSessionRunning();

#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)
    installHandlers();
#endif

    app_.setOrganizationName("EiskaltDC++ Team");
    app_.setApplicationName("EiskaltDC++ Qt");
    app_.setApplicationVersion(QString::fromStdString(eiskaltdcppVersionString));
}

void SessionBootstrap::loadShareData()
{
    dcpp::startupShareData(callBack, nullptr, true);
    HashManager::getInstance()->setPriority(Thread::IDLE);
}

void SessionBootstrap::startUi()
{
    GlobalTimer::newInstance();
    WulforSettings::newInstance();
    WulforSettings::getInstance()->load();
    WulforSettings::getInstance()->loadTheme();
    WulforUtil::newInstance();
    startShareIndex();
    WulforSettings::getInstance()->loadTranslation();
#if defined(Q_OS_MAC)
    WBSET(WB_TRAY_ENABLED, false);
#endif
    loadChrome();
    createWindow();
}

void SessionBootstrap::loadChrome()
{
    Text::hubDefaultCharset =
        WulforUtil::getInstance()->qtEnc2DcEnc(WSGET(WS_DEFAULT_LOCALE)).toStdString();
    if (WulforUtil::getInstance()->loadUserIcons())
        std::cout << QObject::tr("UserList icons has been loaded").toStdString() << std::endl;
    if (WulforUtil::getInstance()->loadIcons())
        std::cout << QObject::tr("Application icons has been loaded").toStdString() << std::endl;
    app_.setWindowIcon(WICON(AppIcons::eiICON_APPL));
}

void SessionBootstrap::createWindow()
{
    ArenaWidgetManager::newInstance();
    MainWindow::newInstance();
#if defined(Q_OS_MAC)
    MainWindow::getInstance()->setUnload(false);
    QObject::connect(&app_, SIGNAL(clickedOnDock()),
                     MainWindow::getInstance(), SLOT(show()));
#else
    MainWindow::getInstance()->setUnload(!WBGET(WB_TRAY_ENABLED));
#endif
    app_.connect(&app_, SIGNAL(messageReceived(QString)),
                 MainWindow::getInstance(), SLOT(parseInstanceLine(QString)));
}

void SessionBootstrap::startServices()
{
    HubManager::newInstance();
    Notification::newInstance();
#ifdef USE_JS
    ScriptEngine::newInstance();
    QObject::connect(ScriptEngine::getInstance(), SIGNAL(scriptChanged(QString)),
                     MainWindow::getInstance(), SLOT(slotJSFileChanged(QString)));
#endif
}

void SessionBootstrap::startDeferredServices()
{
    if (WBGET(WB_ANTISPAM_ENABLED)) {
        AntiSpam::newInstance();
        AntiSpam::getInstance()->loadLists();
        AntiSpam::getInstance()->loadSettings();
    }
    if (WBGET(WB_APP_ENABLE_EMOTICON)) {
        EmoticonFactory::newInstance();
        EmoticonFactory::getInstance()->load();
    }
#ifdef USE_ASPELL
    if (WBGET(WB_APP_ENABLE_ASPELL))
        SpellCheck::newInstance();
#endif
    ArenaWidgetFactory().create<dcpp::Singleton, FinishedUploads>();
    ArenaWidgetFactory().create<dcpp::Singleton, FinishedDownloads>();
    ArenaWidgetFactory().create<dcpp::Singleton, QueuedUsers>();
}

void SessionBootstrap::showAndPaint()
{
    if (!WBGET(WB_MAINWINDOW_HIDE) || !WBGET(WB_TRAY_ENABLED))
        MainWindow::getInstance()->show();

    MainWindow *mw = MainWindow::getInstance();
    if (!mw || !mw->isVisible())
        return;
    // Create the platform window and flush expose before HashIndex.xml /
    // files.xml.bz2 parsing blocks the GUI thread.
    mw->raise();
    (void)mw->winId();
    app_.processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SessionBootstrap::stopUi()
{
    dcpp::noteAppExiting();
#if defined(Q_OS_MAC)
    resignMacAppForExit();
#endif
    std::cout << QObject::tr("Shutting down libeiskaltdcpp...").toStdString() << std::endl;
    dcpp::LogManager::getInstance()->message(_("Application shutting down normally"));
    dcpp::markSessionNormal();
    WulforSettings::getInstance()->save();

    EmoticonFactory::deleteInstance();
#ifdef USE_ASPELL
    if (SpellCheck::getInstance())
        SpellCheck::deleteInstance();
#endif
    Notification::deleteInstance();
#ifdef USE_JS
    ScriptEngine::deleteInstance();
#endif
    GlobalTimer::deleteInstance();
    dcpp::ConnectionManager::getInstance()->shutdown();
    ArenaWidgetManager::deleteInstance();
    HubManager::getInstance()->release();
    MainWindow::deleteInstance();
}

void SessionBootstrap::stopCore()
{
    // DuckDB close and dcpp joins can take seconds; keep pumping Qt so macOS
    // does not show a beachball during exit.
    std::atomic_bool coreDone{false};
    std::thread coreExit([&coreDone] {
        if (dcpp::SearchManager::getInstance())
            dcpp::SearchManager::getInstance()->disconnect();
        stopShareIndex();
        dcpp::shutdown();
        coreDone.store(true, std::memory_order_release);
    });
    pumpUntil(coreDone);
    coreExit.join();

    WulforUtil::deleteInstance();
    WulforSettings::deleteInstance();
    std::cout << QObject::tr("Quit...").toStdString() << std::endl;
}
