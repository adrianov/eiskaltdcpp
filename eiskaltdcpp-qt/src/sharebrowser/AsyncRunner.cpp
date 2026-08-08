/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "sharebrowser/AsyncRunner.h"

AsyncRunner::AsyncRunner(QObject *parent): QThread(parent)
{
}

AsyncRunner::~AsyncRunner()
{
    // QThread::~QThread() aborts if the thread is still running.
    if (isRunning()) {
        quit();
        wait(5000);
        if (isRunning())
            terminate();
        wait(1000);
    }
}

void AsyncRunner::run()
{
    runFunc();
}

void AsyncRunner::setRunFunction(const std::function<void()> &f)
{
    runFunc = f;
}
