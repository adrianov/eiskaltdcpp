/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../MerkleTree.h"
#include "../typedefs.h"

namespace dcpp {

class File;

/**
 * Binary leaf store (HashData.dat): append-only TTH leaves, grown in 1 MiB steps.
 * Index -1 means a single-node tree (root only, no leaves on disk).
 */
class HashDataFile {
public:
    static const int64_t SMALL_TREE = -1;

    static string path();
    /** Migrate legacy path; create an empty 1 MiB store if missing/tiny. */
    static void ensureReady();
    static void create(const string& name);

    /** Append leaves; returns file offset, or SMALL_TREE for a one-leaf tree. */
    static int64_t writeLeaves(File& f, const TigerTree& tt);
    static bool readTree(File& f, int64_t index, int64_t size, int64_t blockSize,
                         const TTHValue& root, TigerTree& out);
};

} // namespace dcpp
