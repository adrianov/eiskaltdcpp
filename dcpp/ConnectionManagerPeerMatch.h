/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "HintedUser.h"

namespace dcpp {
namespace ConnectionManagerPeerMatch {

/** Treat cross-hub NMDC identities as aliases unless stronger metadata disagrees. */
bool samePeer(const HintedUser& a, const HintedUser& b);

} // namespace ConnectionManagerPeerMatch
} // namespace dcpp
