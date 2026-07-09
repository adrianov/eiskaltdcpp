/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)

#include "MainAppUnix.h"

#include "dcpp/stdinc.h"
#include "dcpp/ProcessExit.h"

#if defined(Q_OS_HAIKU)
#include "EiskaltApp_haiku.h"
#elif defined(Q_OS_MAC)
#include "EiskaltApp_mac.h"
#else
#include "EiskaltApp.h"
#endif

#include <QApplication>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#if defined(__GLIBC__)
#include <execinfo.h>

#ifdef ENABLE_STACKTRACE
#include "extra/stacktrace.h"
#endif
#endif

void catchSIG(int sigNum) {
    dcpp::noteFatalSignal(sigNum);
    psignal(sigNum, "Catching signal ");

#ifdef ENABLE_STACKTRACE
    printBacktrace(sigNum);
#endif
    
    EiskaltApp *eapp = dynamic_cast<EiskaltApp*>(qApp);
    
    if (eapp) {
        eapp->getSharedMemory().unlock();
        eapp->getSharedMemory().detach();
    }
    
    raise(SIGINT);
    
    std::abort();
}

template <int sigNum = 0, int ... Params>
void catchSignals() {
    if (!sigNum)
        return;

    psignal(sigNum, "Installing handler for");

    signal(sigNum, catchSIG);

    catchSignals<Params ... >();
}

void installHandlers(){
    dcpp::installSigpipeIgnore();

    catchSignals<SIGSEGV, SIGABRT, SIGBUS, SIGTERM>();

    printf("Signal handlers installed.\n");
}

#endif
