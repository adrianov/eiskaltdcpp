/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 * MediaInfo scan logic adapted from FlylinkDC++ (getMediaInfo).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "MediaInfo.h"
#include "../MerkleTree.h"

#include <string>

namespace dcpp {

/** Scan local media (Flylink getMediaInfo) into MediaInfo; uses TTH cache. */
bool mediaInfoFill(const std::string& path, int64_t size, const TTHValue& tth, MediaInfo& out);

bool isMediaInfoExt(const std::string& ext);

} // namespace dcpp
