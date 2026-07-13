/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <dcpp/stdinc.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/DCPlusPlus.h>
#include "settingsmanager.hh"
#include "wulformanager.hh"
#include "wulforStartup.hh"
#include <iostream>

#define GUI_LOCALE_DIR LOCALE_DIR
#define GUI_PACKAGE "eiskaltdcpp-gtk"

#ifdef ENABLE_STACKTRACE
#include "extra/stacktrace.h"
#endif

int main(int argc, char *argv[])
{
    const int argResult = parseGtkArgs(argc, argv);
    if (argResult == 0)
        return 0;

    bindtextdomain(GUI_PACKAGE, GUI_LOCALE_DIR);
    textdomain(GUI_PACKAGE);
    bind_textdomain_codeset(GUI_PACKAGE, "UTF-8");

    const int session = startGtkSession(argc, argv);
    if (session == 0)
        return 0;

    WulforSettingsManager::newInstance();
    WulforManager::start(argc, argv);
    gdk_threads_enter();
    gtk_main();
    freeGtkSessionConnection();
    gdk_threads_leave();

    // Close hubs/peers before UI teardown uses graceless hub disconnect.
    dcpp::ConnectionManager::getInstance()->shutdown();

    WulforManager::stop();
    WulforSettingsManager::deleteInstance();

    std::cout << _("Shutting down libeiskaltdcpp...") << std::endl;
    dcpp::shutdown();
    std::cout << _("Quit...") << std::endl;
    return 0;
}
