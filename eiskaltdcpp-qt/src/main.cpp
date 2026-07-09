/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifdef BUILD_STATIC
#include <QtPlugin>
#if defined(_WIN32)
Q_IMPORT_PLUGIN (QWindowsAudioPlugin);
Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN (QSQLiteDriverPlugin);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
Q_IMPORT_PLUGIN (QWindowsVistaStylePlugin);
#endif // QT_VERSION
#elif defined(__linux) // defined(_WIN32)
Q_IMPORT_PLUGIN (QXcbIntegrationPlugin);
Q_IMPORT_PLUGIN (QSQLiteDriverPlugin);
#endif // defined(_WIN32)
#endif // BUILD_STATIC

#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;

#include "dcpp/stdinc.h"
#include "dcpp/DCPlusPlus.h"

#include "Notification.h"
#include "MainAppCli.h"
#include "MainAppRun.h"

#if defined(Q_OS_HAIKU)
#include "EiskaltApp_haiku.h"
#elif defined(Q_OS_MAC)
#include "EiskaltApp_mac.h"
#else
#include "EiskaltApp.h"
#endif

#ifdef FORCE_XDG
#include "MainAppXdg.h"
#endif

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QTextCodec>

#include <locale.h>

void callBack(void *, const std::string &a)
{
    std::cout << QObject::tr("Loading: ").toStdString() << a << std::endl;
}

#if defined(Q_OS_MAC)
#include <objc/objc.h>
#include <objc/message.h>

bool dockClickHandler(id self,SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    Notification *N = Notification::getInstance();
    if (N)
        N->slotShowHide();
    return true;
}
#endif

int main(int argc, char *argv[])
{
#if QT_VERSION < 0x050000
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
    setlocale(LC_ALL, "");

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    EiskaltApp app(argc, argv, _q(dcpp::Util::getLoginName()+"EDCPP"));

    parseCmdLine(app.arguments());

    if (app.isRunning()){
        QStringList args = app.arguments();
        args.removeFirst();
#if !defined (Q_OS_HAIKU)
        app.sendMessage(args.join("\n"));
#endif
        return 0;
    }

#if defined(FORCE_XDG) && !defined(Q_OS_WIN)
    migrateConfig();
#endif

    return runApplication(app);
}
