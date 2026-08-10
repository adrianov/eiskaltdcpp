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
    AppPriority yield;
    SessionBootstrap boot(app_);
    boot.bringUp(); // HashIndex / share / queue at normal process priority

    yield.trackWindow(MainWindow::getInstance());
    yield.setYieldAllowed(true);

    const int ret = app_.exec();

    yield.setYieldAllowed(false);
    yield.trackWindow(nullptr);
    boot.tearDown();
    return ret;
}
