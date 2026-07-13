/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * GTK app startup helpers (args, single-instance, core init, signals).
 */

#pragma once

/** @return 0 after printing help/version (caller should exit); -1 to continue. */
int parseGtkArgs(int argc, char *argv[]);

/** @return 0 if this process should exit (secondary instance); 1 to run UI. */
int startGtkSession(int argc, char *argv[]);

void freeGtkSessionConnection();
