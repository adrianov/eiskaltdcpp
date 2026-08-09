/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Session exit tracking and Unix signal setup (SIGPIPE ignore, fatal handlers).
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

#ifndef _WIN32
void installSigpipeIgnore();
void blockSigpipeInThread();
void prepareFatalSignalPath();
void noteFatalSignal(int sig) noexcept;
#endif

void markSessionRunning();
void markSessionNormal() noexcept;
string checkPreviousSession();

/** Mark process quit so socket joins / waits can bail out quickly. */
void noteAppExiting() noexcept;
bool isAppExiting() noexcept;

} // namespace dcpp
