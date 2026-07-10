/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

/** Per-hub reconnect delay from that hub's disconnect count today. */
namespace HubReconnectFilter {

constexpr int MAX_ATTEMPTS = 8;

bool shouldGiveUp(int attempts);
int delaySec(int attempts);
string delayLabel(int attempts);

/** Count a disconnect for hubUrl today; returns today's total for that hub. */
int noteDisconnect(const string& hubUrl);
int todayCount(const string& hubUrl);
/** Manual reconnect / nick retry — start today's count over for this hub. */
void clearToday(const string& hubUrl);

} // namespace HubReconnectFilter

} // namespace dcpp
