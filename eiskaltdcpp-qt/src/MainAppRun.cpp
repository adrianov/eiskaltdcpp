/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "MainAppRun.h"
#include "MainAppUnix.h"

#include "dcpp/stdinc.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/DCPlusPlus.h"
#include "dcpp/HashManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/ProcessExit.h"
#include "dcpp/Thread.h"
#include "dcpp/Singleton.h"

#include "WulforUtil.h"
#include "WulforSettings.h"
#include "HubManager.h"
#include "Notification.h"
#include "VersionGlobal.h"
#include "EmoticonFactory.h"
#include "FinishedTransfers.h"
#include "QueuedUsers.h"
#include "ArenaWidgetManager.h"
#include "ArenaWidgetFactory.h"
#include "MainWindow.h"
#include "GlobalTimer.h"
#include "MainAppShareIndex.h"

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

#include <QObject>
#include <QTextCodec>

#include <iostream>
#include <string>

extern void callBack(void *, const std::string &);

int runApplication(EiskaltApp &app)
{
    int ret = 0;

    dcpp::startup(callBack, nullptr);
    dcpp::TimerManager::getInstance()->start();

    {
        const string prev = dcpp::checkPreviousSession();
        if(!prev.empty())
            dcpp::LogManager::getInstance()->message(prev);
        dcpp::markSessionRunning();
    }

#if !defined (Q_OS_WIN) && !defined (Q_OS_HAIKU)
    installHandlers();
#endif

    HashManager::getInstance()->setPriority(Thread::IDLE);
#if QT_VERSION < 0x050000
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
    app.setOrganizationName("EiskaltDC++ Team");
    app.setApplicationName("EiskaltDC++ Qt");
    app.setApplicationVersion(QString::fromStdString(eiskaltdcppVersionString));
    
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

    Text::hubDefaultCharset = WulforUtil::getInstance()->qtEnc2DcEnc(WSGET(WS_DEFAULT_LOCALE)).toStdString();

    if (WulforUtil::getInstance()->loadUserIcons())
        std::cout << QObject::tr("UserList icons has been loaded").toStdString() << std::endl;

    if (WulforUtil::getInstance()->loadIcons())
        std::cout << QObject::tr("Application icons has been loaded").toStdString() << std::endl;

    app.setWindowIcon(WICON(WulforUtil::eiICON_APPL));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 1))
    app.setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    ArenaWidgetManager::newInstance();

    MainWindow::newInstance();
#if defined(Q_OS_MAC)
    MainWindow::getInstance()->setUnload(false);
    QObject::connect(&app, SIGNAL(clickedOnDock()),
                     MainWindow::getInstance(), SLOT(show()));
#else
    MainWindow::getInstance()->setUnload(!WBGET(WB_TRAY_ENABLED));
#endif

    app.connect(&app, SIGNAL(messageReceived(QString)), MainWindow::getInstance(), SLOT(parseInstanceLine(QString)));

    HubManager::newInstance();

    WulforSettings::getInstance()->loadTheme();

    if (WBGET(WB_APP_ENABLE_EMOTICON)){
        EmoticonFactory::newInstance();
        EmoticonFactory::getInstance()->load();
    }

#ifdef USE_ASPELL
    if (WBGET(WB_APP_ENABLE_ASPELL))
        SpellCheck::newInstance();
#endif

    Notification::newInstance();

#ifdef USE_JS
    ScriptEngine::newInstance();
    QObject::connect(ScriptEngine::getInstance(), SIGNAL(scriptChanged(QString)), MainWindow::getInstance(), SLOT(slotJSFileChanged(QString)));
#endif

    ArenaWidgetFactory().create< dcpp::Singleton, FinishedUploads >();
    ArenaWidgetFactory().create< dcpp::Singleton, FinishedDownloads >();
    ArenaWidgetFactory().create< dcpp::Singleton, QueuedUsers >();

    MainWindow::getInstance()->autoconnect();
    MainWindow::getInstance()->parseCmdLine(app.arguments());

    if (!WBGET(WB_MAINWINDOW_HIDE) || !WBGET(WB_TRAY_ENABLED))
        MainWindow::getInstance()->show();

    ret = app.exec();

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

    // Close hubs/peers while still fully alive (before hub widgets use disconnect(true)).
    dcpp::ConnectionManager::getInstance()->shutdown();

    ArenaWidgetManager::deleteInstance();
    
    HubManager::getInstance()->release();

    MainWindow::deleteInstance();

    stopShareIndex();

    WulforUtil::deleteInstance();

    WulforSettings::deleteInstance();

    dcpp::shutdown();

    std::cout << QObject::tr("Quit...").toStdString() << std::endl;

    return ret;
}
