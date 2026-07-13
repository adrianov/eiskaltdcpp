/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include <dcpp/TimerManager.h>
#include "bacon-message-connection.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"
#include "wulforStartup.hh"
#include <iostream>
#include <signal.h>
#include <string.h>

#include "VersionGlobal.h"

#ifdef ENABLE_STACKTRACE
#include "extra/stacktrace.h"
#endif

#define GUI_PACKAGE "eiskaltdcpp-gtk"

namespace {

void printHelp()
{
    printf(_(
               "Usage:\n"
               "  eiskaltdcpp-gtk <magnet link> <dchub://link> <adc(s)://link>\n"
               "  eiskaltdcpp-gtk <Key>\n"
               "EiskaltDC++ is a cross-platform program that uses the Direct Connect and ADC protocols.\n"
               "\n"
               "Keys:\n"
               "  -h, --help\t Show this message\n"
               "  -V, --version\t Show version string\n"
               )
           );
}

void printVersion()
{
    printf("%s version: %s\n", eiskaltdcppAppNameString.c_str(), eiskaltdcppVersionString.c_str());
    printf("GTK+ version: %d.%d.%d\n", gtk_major_version, gtk_minor_version, gtk_micro_version);
    printf("Glib version: %d.%d.%d\n", glib_major_version, glib_minor_version, glib_micro_version);
}

void receiver(const char *link, gpointer data)
{
    (void)data;
    g_return_if_fail(link != NULL);
    WulforManager::get()->onReceived_gui(link);
}

void callBack(void *, const std::string &a)
{
    std::cout << _("Loading: ") << a << std::endl;
}

void catchSIG(int sigNum) {
    psignal(sigNum, _("Catching signal "));

#ifdef ENABLE_STACKTRACE
    printBacktrace(sigNum);
#endif

    raise(SIGINT);
    std::abort();
}

template <int sigNum = 0, int ... Params>
void catchSignals() {
    if (!sigNum)
        return;

    psignal(sigNum, _("Installing handler for"));
    signal(sigNum, catchSIG);
    catchSignals<Params ... >();
}

void installHandlers(){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &sa, NULL) == -1)
        printf(_("Cannot handle SIGPIPE\n"));
    else {
        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
    }

    catchSignals<SIGSEGV, SIGABRT, SIGBUS, SIGTERM>();
    printf(_("Signal handlers installed.\n"));
}

} // namespace

BaconMessageConnection *gtkSessionConnection = NULL;

int parseGtkArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++){
        if (!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")){
            printHelp();
            return 0;
        }
        else if (!strcmp(argv[i],"--version") || !strcmp(argv[i],"-V")){
            printVersion();
            return 0;
        }
    }
    return -1;
}

/** @return 0 = exit OK (secondary instance), 1 = continue as primary, <0 unused */
int startGtkSession(int argc, char *argv[])
{
    gtkSessionConnection = bacon_message_connection_new(GUI_PACKAGE);
    dcdebug(gtkSessionConnection ? "eiskaltdcpp-gtk: connection yes...\n" : "eiskaltdcpp-gtk: connection no...\n");

    if (WulforUtil::profileIsLocked())
    {
        if (!bacon_message_connection_get_is_server(gtkSessionConnection))
        {
            dcdebug("eiskaltdcpp-gtk: is client...\n");
            if (argc > 1)
            {
                dcdebug("eiskaltdcpp-gtk: send %s\n", argv[1]);
                bacon_message_connection_send(gtkSessionConnection, argv[1]);
            }
        }
        bacon_message_connection_free(gtkSessionConnection);
        gtkSessionConnection = NULL;
        return 0;
    }

    if (bacon_message_connection_get_is_server(gtkSessionConnection))
    {
        dcdebug("eiskaltdcpp-gtk: is server...\n");
        bacon_message_connection_set_callback(gtkSessionConnection, receiver, NULL);
    }

    dcpp::startup(callBack, NULL);
    dcpp::TimerManager::getInstance()->start();

#if !GLIB_CHECK_VERSION(2,32,0)
    g_thread_init(NULL);
#endif
    gdk_threads_init();
    gtk_init(&argc, &argv);
    g_set_application_name("EiskaltDC++ Gtk");

    installHandlers();
    return 1;
}

void freeGtkSessionConnection()
{
    if (gtkSessionConnection) {
        bacon_message_connection_free(gtkSessionConnection);
        gtkSessionConnection = NULL;
    }
}
