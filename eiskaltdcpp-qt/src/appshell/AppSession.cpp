/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "appshell/AppSession.h"
#include "appshell/AppPriority.h"
#include "appshell/session/SessionBootstrap.h"
#include "MainWindow.h"

#if defined(Q_OS_HAIKU)
#include "EiskaltApp_haiku.h"
#elif defined(Q_OS_MAC)
#include "EiskaltApp_mac.h"
#else
#include "EiskaltApp.h"
#endif

AppSession::AppSession(EiskaltApp &app)
    : app_(app)
{
}

int AppSession::run()
{
    SessionBootstrap boot(app_);
    boot.bringUp();

    AppPriority yield;
    yield.trackWindow(MainWindow::getInstance());

    const int ret = app_.exec();

    yield.restoreNormal();
    yield.trackWindow(nullptr);
    boot.tearDown();
    return ret;
}
