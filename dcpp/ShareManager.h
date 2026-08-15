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

#include <unordered_map>
#include <utility>

#include "TimerManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "HashManagerListener.h"
#include "QueueManagerListener.h"
#include "Exception.h"
#include "CriticalSection.h"
#include "Singleton.h"
#include "BloomFilter.h"
#include "Atomic.h"
#include "share/ShareFileList.h"
#include "share/ShareDirectory.h"

#ifdef WITH_DHT
namespace dht {
class IndexManager;
}
#endif

namespace dcpp {

STANDARD_EXCEPTION(ShareException);

class SimpleXML;
class Client;
class File;
class OutputStream;
class MemoryInputStream;

struct ShareLoader;
class ShareManager : public Singleton<ShareManager>, private SettingsManagerListener, private Thread,
        private TimerManagerListener, private HashManagerListener, private QueueManagerListener
{
public:
    void addDirectory(const string& realPath, const string &virtualName);
    void removeDirectory(const string& realPath);
    void removeFile(const string& realPath) noexcept;
    /** Put realPath back in tthIndex when the previous indexed copy was removed. */
    void indexFile(const string& realPath) noexcept;
    /** True when realPath is a folder nested inside a share, not a share root or its parent. */
    bool isNestedShareDir(const string& realPath) const noexcept;
    /** Remove a nested shared directory from the in-memory index. Does not delete disk or share roots. */
    void removeDir(const string& realPath) noexcept;
    /** Rename a shared folder in the in-memory index after it was renamed on disk. */
    bool renameDir(const string& realPath, const string& newName) noexcept;
    void renameDirectory(const string& realPath, const string& virtualName);

    bool isRefreshing() const { return refreshing; }

    string toVirtual(const TTHValue& tth) const;
    string toReal(const string& virtualFile);
    StringList getRealPaths(const string& virtualPath);
    TTHValue getTTH(const string& virtualFile) const;

    void refresh(bool dirs = false, bool aUpdate = true, bool block = false) noexcept;
    void setDirty() { fileList.setDirty(); }

    void search(SearchResultList& l, const string& aString, int aSearchType, int64_t aSize, int aFileType,
                Client* aClient, StringList::size_type maxResults) noexcept;
    void search(SearchResultList& l, const StringList& params, StringList::size_type maxResults) noexcept;

    StringPairList getDirectories() const noexcept;

    MemoryInputStream* generatePartialList(const string& dir, bool recurse) const {
        return fileList.generatePartial(dir, recurse);
    }
    MemoryInputStream* getTree(const string& virtualFile) const;

    AdcCommand getFileInfo(const string& aFile);

    int64_t getShareSize() const noexcept;
    int64_t getShareSize(const string& realPath) const noexcept;
    size_t getSharedFiles() const noexcept;

    string getShareSizeString() const { return Util::toString(getShareSize()); }
    string getShareSizeString(const string& aDir) const { return Util::toString(getShareSize(aDir)); }

    void getBloom(ByteVector& v, size_t k, size_t m, size_t h) const;
    SearchManager::TypeModes getType(const string& fileName) const noexcept;

    string validateVirtual(const string& /*aVirt*/) const noexcept;
    bool hasVirtual(const string& name) const noexcept;

    void addHits(uint32_t aHits) { hits += aHits; }
    const string getOwnListFile() { return fileList.ensure(); }
    const string& getBZXmlFile() const { return fileList.getPath(); }

    bool isTTHShared(const TTHValue& tth) {
        Lock l(cs);
        return tthIndex.find(tth) != tthIndex.end();
    }
    void publish();

    GETSET(uint32_t, hits, Hits);

private:
    using Directory = ShareDirectory;
    using AdcSearch = ShareAdcSearch;
    using DirList = ShareDirList;
    using HashFileMap = ShareHashMap;

    friend class ShareDirectory;
    friend struct ShareLoader;
    friend class ShareFileList;
    friend class ShareTreeScan;
    friend class Singleton<ShareManager>;
#ifdef WITH_DHT
    friend class ::dht::IndexManager;
#endif

    ShareManager();
    virtual ~ShareManager();

    bool refreshDirs;
    bool update;
    bool initial;
    Atomic<bool, memory_ordering_strong> refreshing;
    uint64_t lastFullUpdate;
    ShareFileList fileList;
    mutable CriticalSection cs;

    DirList directories;
    /** Map real name to virtual name - multiple real names may be mapped to a single virtual one */
    StringMap shares;
    HashFileMap tthIndex;
    BloomFilter<5> bloom;

    Directory::File::Set::const_iterator findFile(const string& virtualFile) const;
    Directory::Ptr buildTree(const string& aName, const Directory::Ptr& aParent);
    bool checkHidden(const string& aName) const;
    void rebuildIndices();
    void updateIndices(Directory& aDirectory);
    void updateIndices(Directory& dir, const decltype(std::declval<Directory>().files.begin())& i);
    Directory::Ptr merge(const Directory::Ptr& directory);
    DirList::const_iterator getByVirtual(const string& virtualName) const noexcept;
    pair<Directory::Ptr, string> splitVirtual(const string& virtualPath) const;
    string findRealRoot(const string& virtualRoot, const string& virtualLeaf) const;
    Directory::Ptr getDirectory(const string& fname);

    virtual int run();
    virtual void on(QueueManagerListener::FileMoved, const string& realPath) noexcept;
    virtual void on(HashManagerListener::TTHDone, const string& realPath, const TTHValue& root) noexcept;
    virtual void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept { save(xml); }
    virtual void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept { load(xml); }
    virtual void on(TimerManagerListener::Minute, uint64_t tick) noexcept;
    void load(SimpleXML& aXml);
    void save(SimpleXML& aXml);
};

} // namespace dcpp
