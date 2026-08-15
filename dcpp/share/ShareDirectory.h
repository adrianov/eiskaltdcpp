/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include <algorithm>
#include <list>
#include <set>
#include <unordered_map>

#include "../NonCopyable.h"
#include "../FastAlloc.h"
#include "../Pointer.h"
#include "../MerkleTree.h"
#include "../GetSet.h"
#include "../Util.h"
#include "../SearchManager.h"
#include "../SettingsManager.h"
#include "../StringSearch.h"
#include "../sharemedia/MediaInfo.h"

namespace dcpp {

using std::set;
using std::unordered_map;

class Client;
class OutputStream;
struct ShareAdcSearch;

/** In-memory share tree node: virtual name, nested dirs, and hashed files. */
class ShareDirectory : public FastAlloc<ShareDirectory>,
                       public intrusive_ptr_base<ShareDirectory>,
                       private NonCopyable {
public:
    typedef dcpp::intrusive_ptr<ShareDirectory> Ptr;
    typedef unordered_map<string, Ptr, CaseStringHash, CaseStringEq> Map;
    typedef Map::iterator MapIter;

    struct File {
        File() : size(0), parent(0), ts(0) { }
        File(const string& aName, int64_t aSize, const Ptr& aParent, const TTHValue& aRoot) :
            name(aName), tth(aRoot), size(aSize), parent(aParent.get()), ts(0) { }
        File(const File& rhs) :
            name(rhs.getName()), tth(rhs.getTTH()), size(rhs.getSize()), parent(rhs.getParent()),
            mediaInfo(rhs.mediaInfo), ts(rhs.ts) { }

        File& operator=(const File& rhs) {
            name = rhs.name; size = rhs.size; parent = rhs.parent; tth = rhs.tth;
            mediaInfo = rhs.mediaInfo; ts = rhs.ts;
            return *this;
        }

        bool operator==(const File& rhs) const {
            if (BOOLSETTING(CASESENSITIVE_FILELIST))
                return getParent() == rhs.getParent() && (strcmp(getName().c_str(), rhs.getName().c_str()) == 0);
            return getParent() == rhs.getParent() && (Util::stricmp(getName(), rhs.getName()) == 0);
        }

        struct StringComp {
            StringComp(const string& s) : a(s) { }
            bool operator()(const File& b) const {
                if (BOOLSETTING(CASESENSITIVE_FILELIST))
                    return strcmp(a.c_str(), b.getName().c_str()) == 0;
                return Util::stricmp(a, b.getName()) == 0;
            }
            const string& a;
        private:
            StringComp& operator=(const StringComp&);
        };

        struct FileLess {
            bool operator()(const File& a, const File& b) const {
                if (BOOLSETTING(CASESENSITIVE_FILELIST))
                    return (strcmp(a.getName().c_str(), b.getName().c_str()) < 0);
                return (Util::stricmp(a.getName(), b.getName()) < 0);
            }
        };

        typedef set<File, FileLess> Set;

        string getADCPath() const { return parent->getADCPath() + name; }
        string getFullName() const { return parent->getFullName() + name; }
        string getRealPath() const { return parent->getRealPath(name); }

        GETSET(string, name, Name);
        GETSET(TTHValue, tth, TTH);
        GETSET(int64_t, size, Size);
        GETSET(ShareDirectory*, parent, Parent);
        GETSET(uint32_t, ts, TS);
        MediaInfo mediaInfo;
    };

    int64_t size;
    Map directories;
    set<File, File::FileLess> files;

    static Ptr create(const string& aName, const Ptr& aParent = Ptr()) {
        return Ptr(new ShareDirectory(aName, aParent));
    }

    bool hasType(uint32_t type) const noexcept {
        if (type == SearchManager::TYPE_ANY)
            return true;
        if (type == SearchManager::TYPE_AUDIO_VIDEO)
            return (fileTypes & ((1 << SearchManager::TYPE_AUDIO) | (1 << SearchManager::TYPE_VIDEO))) != 0;
        return (fileTypes & (1 << type)) != 0;
    }
    void addType(uint32_t type) noexcept;

    string getADCPath() const noexcept;
    string getFullName() const noexcept;
    string getRealPath(const std::string& path) const;
    int64_t getSize() const noexcept;

    void search(SearchResultList& aResults, StringSearch::List& aStrings, int aSearchType, int64_t aSize,
                int aFileType, Client* aClient, StringList::size_type maxResults) const noexcept;
    void search(SearchResultList& aResults, ShareAdcSearch& aStrings,
                StringList::size_type maxResults) const noexcept;

    void toXml(OutputStream& xmlFile, string& indent, string& tmp2, bool fullList) const;
    void filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const;

    auto findFile(const string& aFile) const -> decltype(files.cbegin()) {
        return std::find_if(files.begin(), files.end(), File::StringComp(aFile));
    }

    void merge(const Ptr& source);

    GETSET(string, name, Name);
    GETSET(ShareDirectory*, parent, Parent);

private:
    friend void intrusive_ptr_release(intrusive_ptr_base<ShareDirectory>*);
    friend class ShareManager;

    ShareDirectory(const string& aName, const Ptr& aParent);
    ~ShareDirectory() { }

    void searchAdcFiles(SearchResultList& results, ShareAdcSearch& query, StringSearch::List* terms,
                        StringList::size_type maxResults) const noexcept;

    uint32_t fileTypes;
};

/** ADC file/directory search terms matched against a ShareDirectory tree. */
struct ShareAdcSearch {
    ShareAdcSearch(const StringList& adcParams);

    bool isExcluded(const string& str);
    bool hasExt(const string& name);
    StringSearch::List* include;
    StringSearch::List includeInit;
    StringSearch::List exclude;
    StringList ext;
    StringList noExt;

    int64_t gt;
    int64_t lt;

    TTHValue root;
    bool hasRoot;
    bool isDirectory;
};

typedef std::list<ShareDirectory::Ptr> ShareDirList;
typedef unordered_map<TTHValue, ShareDirectory::File::Set::const_iterator> ShareHashMap;

} // namespace dcpp
