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

#include <functional>
#include <unordered_set>

namespace dcpp {
namespace ConnectionManagerPeerMatch {

/** Same nick or IP, same share size, different hub — same user and file list. */
bool samePeer(const HintedUser& a, const HintedUser& b);

/** Invoke fn for seed and every online cross-hub alias of seed. */
void forEachListPeer(const HintedUser& seed, const std::function<void(const HintedUser&)>& fn);

/** Backoff keys for this identity (nick/IP + share). Aliases collide on the same keys. */
void collectListPeerKeys(const HintedUser& user, std::unordered_set<string>& keys);

} // namespace ConnectionManagerPeerMatch
} // namespace dcpp
