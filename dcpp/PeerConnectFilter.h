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

} // namespace PeerConnectFilter

} // namespace dcpp
