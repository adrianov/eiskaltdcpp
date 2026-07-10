/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include "ClientManagerListener.h"
#include "CriticalSection.h"
#include "DirectoryListing.h"
#include "Exception.h"
#include "File.h"
#include "MerkleTree.h"
#include "QueueItem.h"
#include "QueueManagerIndex.h"
#include "QueueManagerListener.h"
#include "QueueManagerWorkers.h"
#include "SearchManagerListener.h"
#include "Singleton.h"
#include "TimerManager.h"
#include "User.h"

namespace dcpp {

STANDARD_EXCEPTION(QueueException);

class UserConnection;
class ConnectionQueueItem;
class QueueLoader;

class QueueManager : public Singleton<QueueManager>, public Speaker<QueueManagerListener>, private TimerManagerListener,
        private SearchManagerListener, private ClientManagerListener
{
public:
    struct SourceMatch {
        SourceMatch(const TTHValue& tth_, int64_t size_) : tth(tth_), size(size_) { }
        TTHValue tth;
        int64_t size;
    };

    void add(const string& aTarget, int64_t aSize, const TTHValue& root); // NOTE: freedcpp

    /** Add a file to the queue. */
    void add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
             int aFlags = 0, bool addBad = true);
    /** Add a user's filelist to the queue. */
    void addList(const HintedUser& HintedUser, int aFlags, const string& aInitialDir = Util::emptyString);
    /** True when this user's full file list is already queued. */
    bool hasListQueued(const HintedUser& user) noexcept;
    /** Count unfinished FLAG_USER_LIST items in the download queue. */
    size_t countQueuedLists() noexcept;
    string getListPath(const HintedUser& user);
    /** Readd a source that was removed */
    void readd(const string& target, const HintedUser& aUser);
    /** Add a directory to the queue (downloads filelist and matches the directory). */
    void addDirectory(const string& aDir, const HintedUser& aUser, const string& aTarget,
                      QueueItem::Priority p = QueueItem::DEFAULT) noexcept;

    int matchListing(const DirectoryListing& dl) noexcept;
    void matchAllListings();
    /** Unfinished queue roots used for indexed source discovery. */
    vector<TTHValue> getQueuedTTHs() noexcept;
    /** Add verified TTH+size matches through the normal queue and connection path. */
    void matchSources(const HintedUser& user, const vector<SourceMatch>& matches) noexcept;

    bool getTTH(const string& name, TTHValue& tth) noexcept;

    int64_t getSize(const string& target) noexcept;
    int64_t getPos(const string& target) noexcept;
    void getSizeInfo(int64_t& size, int64_t& pos, const string& target) noexcept;

    /** Move the target location of a queued item. Running items are silently ignored */
    void move(const string& aSource, const string& aTarget) noexcept;

    void remove(const string& aTarget) noexcept;
    /** Remove file-list queue items (by FLAG_USER_LIST or FileLists/ path). Disk cache unchanged. */
    void removeUserLists() noexcept;
    void removeSource(const string& aTarget, const UserPtr& aUser, int reason, bool removeConn = true) noexcept;
    void removeSource(const UserPtr& aUser, int reason) noexcept;

    void recheck(const string& aTarget);

    void setPriority(const string& aTarget, QueueItem::Priority p) noexcept;

    StringList getTargets(const TTHValue& tth);
    QueueItem::StringMap& lockQueue() noexcept { cs.lock(); return fileQueue.getQueue(); }
    void unlockQueue() noexcept { cs.unlock(); }

    Download* getDownload(UserConnection& aSource, bool supportsTrees) noexcept;
    void putDownload(Download* aDownload, bool finished) noexcept;
    void setFile(Download* download);
    void handleDiskFull(const string& target) noexcept;

    int64_t getQueued(const UserPtr& aUser) const;

    /** @return The highest priority download the user has, PAUSED may also mean no downloads */
    QueueItem::Priority hasDownload(const UserPtr& aUser) noexcept;

    bool getQueueInfo(const UserPtr& aUser, string& aTarget, int64_t& aSize, int& aFlags) noexcept;

    int countOnlineSources(const string& aTarget);

    void loadQueue() noexcept;
    void saveQueue(bool force = false) noexcept;

    void noDeleteFileList(const string& path);

    bool handlePartialSearch(const TTHValue& tth, PartsInfo& _outPartsInfo);
    bool handlePartialResult(const UserPtr& aUser, const string& hubHint, const TTHValue& tth, const QueueItem::PartialSource& partialSource, PartsInfo& outPartialInfo);

    bool isChunkDownloaded(const TTHValue& tth, int64_t startPos, int64_t& bytes, string& tempTarget, int64_t& size) {
        Lock l(cs);
        auto ql = fileQueue.find(tth);

        if(ql.empty()) return false;

        QueueItem* qi = ql.front();

        tempTarget = qi->getTempTarget();
        size = qi->getSize();

        return qi->isChunkDownloaded(startPos, bytes);
    }

    GETSET(uint64_t, lastSave, LastSave);
    GETSET(string, queueFile, QueueFile);
private:
    friend class FileMover;
    friend class Rechecker;
    friend class QueueLoader;
    friend class Singleton<QueueManager>;

    static const int64_t MOVER_LIMIT = 10*1024*1024;

    FileMover mover;
    Rechecker rechecker;

    QueueManager();
    virtual ~QueueManager();

    mutable CriticalSection cs;

    FileQueue fileQueue;
    UserQueue userQueue;
    DirectoryItem::DirectoryMap directories;
    StringList recent;
    StringList recentNames;
    bool dirty;
    uint64_t nextSearch;
    bool nextAutoSearchTTH;
    StringList protectedFileLists;

    static string checkTarget(const string& aTarget, bool checkExsistence);
    bool addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad);

    void processList(const string& name, const HintedUser& user, int flags);
    bool tryUseCachedList(const HintedUser& user, int flags, const string& initialDir);
    void purgeOtherListQueues(const HintedUser& aUser);

    void load(const SimpleXML& aXml);
    void moveFile(const string& source, const string& target);
    static void moveFile_(const string& source, const string& target);
    void moveStuckFile(QueueItem* qi);
    void rechecked(QueueItem* qi);

    void setDirty();

    bool checkSfv(QueueItem* qi, Download* d);
    uint32_t calcCrc32(const string& file);

    void logFinishedDownload(QueueItem* qi, Download* d, bool crcChecked);

    virtual void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
    virtual void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;

    virtual void on(SearchManagerListener::SR, const SearchResultPtr&) noexcept;

    virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
    virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;
};

} // namespace dcpp
