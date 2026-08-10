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

#include "Exception.h"
#include "Singleton.h"
#include "Speaker.h"
#include "MerkleTree.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "TimerManager.h"
#include "HashManagerListener.h"

#include "hash/HashWorker.h"
#include "hash/HashStore.h"
#include "hash/StreamStore.h"

namespace dcpp {

STANDARD_EXCEPTION(HashException);

class HashManager : public Singleton<HashManager>, public Speaker<HashManagerListener>,
        private TimerManagerListener
{
public:
    /** We don't keep leaves for blocks smaller than this... */
    static const int64_t MIN_BLOCK_SIZE;

    HashManager();
    virtual ~HashManager();

    /**
     * Check if the TTH tree associated with the filename is current.
     */
    bool checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp);

    void stopHashing(const string& baseDir) { hasher.stopHashing(baseDir); }
    void setPriority(Thread::Priority p) { hasher.setThreadPriority(p); }

    /** @return TTH root */
    TTHValue getTTH(const string& aFileName, int64_t aSize);

    /** eiskaltdc++ **/
    const TTHValue* getFileTTHif(const string& aFileName);

    bool getTree(const TTHValue& root, TigerTree& tt);

    /** Return block size of the tree associated with root, or 0 if no such tree is in the store */
    int64_t getBlockSize(const TTHValue& root);

    void addTree(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tt) {
        hashDone(aFileName, aTimeStamp, tt, -1, -1);
    }
    void addTree(const TigerTree& tree) { Lock l(cs); store.addTree(tree); }

    void getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const {
        hasher.getStats(curFile, bytesLeft, filesLeft);
    }

    void rebuild() { hasher.scheduleRebuild(); }

    void startup() { startHasher(); loadDatabase(); }
    /** Start the hasher thread; HashIndex.xml is loaded separately via loadDatabase(). */
    void startHasher() { hasher.start(); }
    /** Parse HashIndex.xml (often multi-MB — call after first UI paint when possible). */
    void loadDatabase() { Lock l(cs); store.load(); }

    void shutdown() {
        hasher.shutdown();
        hasher.join();
        Lock l(cs);
        store.save();
    }

    struct HashPauser {
        HashPauser();
        ~HashPauser();
    private:
        bool resume;
    };

    /// @return whether hashing was already paused
    bool pauseHashing() noexcept;
    void resumeHashing() noexcept;
    bool isHashingPaused() const noexcept;

private:
    friend class HashWorker;

    HashWorker hasher;
    HashStore store;
    StreamStore m_streamstore;

    mutable CriticalSection cs;

    void hashDone(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, int64_t speed, int64_t size);

    void doRebuild() {
        Lock l(cs);
        store.rebuild();
    }

    virtual void on(TimerManagerListener::Minute, uint64_t) noexcept {
        Lock l(cs);
        store.save();
    }
    void on(TimerManagerListener::Second, uint64_t) noexcept;
};

} // namespace dcpp
