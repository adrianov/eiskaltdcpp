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

#ifdef USE_XATTR
#include "attr/attributes.h"
#else
#define ATTR_MAX_VALUELEN 128
#endif

namespace dcpp {

/** Optional per-file TTH sidecar (xattr / .gltth stream metadata). */
class StreamStore {
public:
    struct TTHStreamHeader {
        uint32_t magic = 0;
        uint32_t checksum = 0;
        uint64_t fileSize = 0;
        uint64_t timeStamp = 0;
        uint64_t blockSize = 0;
        TTHValue root;
    };

    bool loadTree(const string& p_filePath, TigerTree& tree, int64_t p_aFileSize = -1);
    bool saveTree(const string& p_filePath, const TigerTree& p_Tree);
    void deleteStream(const string& p_filePath);

private:
    static const uint32_t g_MAGIC = 0x2b2b6c67;
    static const string g_streamName;

    void setCheckSum(TTHStreamHeader& p_header);
    bool validateCheckSum(const TTHStreamHeader& p_header);
};

} // namespace dcpp
