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

#include <unordered_map>
#include <vector>

#include "../GetSet.h"
#include "../MerkleTree.h"
#include "../typedefs.h"
#include "HashDataFile.h"

namespace dcpp {

class File;
class HashIndexXml;
class HashIndexXmlLoader;

/** In-memory TTH maps backed by HashIndex.xml + HashData.dat. */
class HashStore {
public:
    static const int64_t SMALL_TREE = HashDataFile::SMALL_TREE;

    HashStore();
    void addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed);
    /** Remap file-index keys after a directory was renamed on disk. */
    void renameDir(const string& oldPath, const string& newPath);

    void load();
    void save();
    void rebuild();

    bool checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp);
    const TTHValue* getTTH(const string& aFileName);

    void addTree(const TigerTree& tt) noexcept;
    bool getTree(const TTHValue& root, TigerTree& tth);
    int64_t getBlockSize(const TTHValue& root) const;
    bool isDirty() { return dirty; }

private:
    struct TreeInfo {
        TreeInfo() : size(0), index(0), blockSize(0) { }
        TreeInfo(int64_t aSize, int64_t aIndex, int64_t aBlockSize) :
            size(aSize), index(aIndex), blockSize(aBlockSize) { }

        GETSET(int64_t, size, Size);
        GETSET(int64_t, index, Index);
        GETSET(int64_t, blockSize, BlockSize);
    };

    struct FileInfo {
        FileInfo(const string& aFileName, const TTHValue& aRoot, uint32_t aTimeStamp, bool aUsed) :
            fileName(aFileName), root(aRoot), timeStamp(aTimeStamp), used(aUsed) { }

        bool operator==(const string& name) const { return name == fileName; }

        GETSET(string, fileName, FileName);
        GETSET(TTHValue, root, Root);
        GETSET(uint32_t, timeStamp, TimeStamp);
        GETSET(bool, used, Used);
    };

    friend class HashIndexXml;
    friend class HashIndexXmlLoader;

    unordered_map<string, vector<FileInfo>> fileIndex;
    unordered_map<TTHValue, TreeInfo> treeIndex;
    bool dirty;
};

} // namespace dcpp
