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
Q_IMPORT_PLUGIN (QWindowsVistaStylePlugin);
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
#include "appshell/MainAppCli.h"
#include "appshell/MainAppRun.h"

#if defined(Q_OS_HAIKU)
#include "EiskaltApp_haiku.h"
#elif defined(Q_OS_MAC)
#include "EiskaltApp_mac.h"
#else
#include "EiskaltApp.h"
#endif

#ifdef FORCE_XDG
#include "appshell/MainAppXdg.h"
#endif

#include <QGuiApplication>
#include <QObject>

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
    setlocale(LC_ALL, "");

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

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
