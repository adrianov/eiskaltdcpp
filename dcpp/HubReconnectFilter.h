/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

/** Per-hub reconnect delay from today's disconnect count. */
namespace HubReconnectFilter {

constexpr int MAX_ATTEMPTS = 8;
/** Wait after manual Reconnect so the hub can release the old session. */
constexpr int MANUAL_DELAY_SEC = 5;

bool shouldGiveUp(int attempts);
int delaySec(int attempts);
string delayLabel(int attempts);
/** Label for MANUAL_DELAY_SEC (gettext). */
string manualDelayLabel();

/** Count a disconnect; returns today's total for hubUrl. */
int noteDisconnect(const string& hubUrl);
int todayCount(const string& hubUrl);
/** Manual Reconnect / nick retry — reset today's count for hubUrl. */
void clearToday(const string& hubUrl);

} // namespace HubReconnectFilter

} // namespace dcpp
