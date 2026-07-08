/*
 * Session exit tracking and Unix signal setup (SIGPIPE ignore, fatal handlers).
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

#ifndef _WIN32
void installSigpipeIgnore();
void blockSigpipeInThread();
void noteFatalSignal(int sig) noexcept;
#endif

void markSessionRunning();
void markSessionNormal() noexcept;
string checkPreviousSession();

} // namespace dcpp
