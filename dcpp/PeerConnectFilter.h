/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"

namespace dcpp {

class OnlineUser;

namespace PeerConnectFilter {

/** Skip hub ghosts (bot flag, empty client tag, stale search entries). */
bool isViablePeer(const OnlineUser& ou);

/** Passive StrgDC++/Flylink-style clients expect $RevConnectToMe first. */
bool prefersRevConnect(const OnlineUser& ou);

constexpr int MAX_CONNECT_ERRORS = 6;
constexpr int MAX_SLOT_WAITS = 12;
constexpr int SLOT_WAIT_BASE_MS = 5 * 60 * 1000;

bool shouldGiveUp(int errors);
bool shouldGiveUpSlotWait(int slotWaits);
int connectBackoffMs(int errors);
int slotWaitBackoffMs(int slotWaits);
int queueBackoffMs(int errors, int slotWaits);
bool shouldLogTimeout(int errors);
bool shouldLogSlotWait(int slotWaits);

} // namespace PeerConnectFilter

} // namespace dcpp
