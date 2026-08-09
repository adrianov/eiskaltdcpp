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

#include <deque>
#include <map>
#include <utility>

#include "../CriticalSection.h"
#include "../MerkleTree.h"
#include "../Semaphore.h"
#include "../Thread.h"
#include "../typedefs.h"

namespace dcpp {

class CRC32Filter;
class HashManager;

/**
 * Background worker: hashes shared files from a sorted queue, defers paths
 * that are writer-held or whose meta changed mid-pass, then retries after idle.
 */
class HashWorker : public Thread {
public:
    HashWorker() : stop(false), running(false), paused(0), rebuild(false), currentSize(0) { }

    void setOwner(HashManager* m) noexcept { owner = m; }

    void hashFile(const string& fileName, int64_t size) noexcept;

    bool pause() noexcept;
    void resume() noexcept;
    bool isPaused() const noexcept;

    void stopHashing(const string& baseDir);
    virtual int run();
    bool fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size, CRC32Filter* xcrc32);
    void getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const;
    void shutdown() { stop = true; if (paused) { s.signal(); resume(); } s.signal(); }
    void scheduleRebuild() { rebuild = true; if (paused) s.signal(); s.signal(); }

private:
    using WorkItem = pair<string, int64_t>;

    map<string, int64_t> w;
    deque<WorkItem> deferred;
    mutable CriticalSection cs;
    Semaphore s;

    HashManager* owner = nullptr;
    bool stop;
    bool running;
    unsigned paused;
    bool rebuild;
    string currentFile;
    int64_t currentSize;

    void instantPause();
    void deferFile(const string& fileName, int64_t size) noexcept;
    bool promoteDeferred() noexcept;
};

} // namespace dcpp
