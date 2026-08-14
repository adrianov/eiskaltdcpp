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

#include "stdinc.h"
#include "ShareManager.h"

#include "ClientManager.h"
#include "File.h"
#include "LogManager.h"
#include "HashBloom.h"
#include "HashManager.h"
#include "sharemedia/MediaInfoCache.h"
#include "QueueManager.h"
#include "SimpleXML.h"
#include "Transfer.h"
#include "UploadManager.h"
#include "UserConnection.h"
#include "share/ShareFileType.h"
#include "share/ShareTreeScan.h"
#ifdef WITH_DHT
#include "dht/IndexManager.h"
#endif

namespace dcpp {

ShareManager::ShareManager() : hits(0),
    refreshDirs(false), update(false), initial(true), refreshing(false),
    lastFullUpdate(GET_TICK()), fileList(*this), bloom(1<<20)
{
    SettingsManager::getInstance()->addListener(this);
    TimerManager::getInstance()->addListener(this);
    QueueManager::getInstance()->addListener(this);
    HashManager::getInstance()->addListener(this);
}

ShareManager::~ShareManager() {
    SettingsManager::getInstance()->removeListener(this);
    TimerManager::getInstance()->removeListener(this);
    QueueManager::getInstance()->removeListener(this);
    HashManager::getInstance()->removeListener(this);

    join();
}

ShareManager::Directory::Directory(const string& aName, const ShareManager::Directory::Ptr& aParent) :
    size(0),
    name(aName),
    parent(aParent.get()),
    fileTypes(1 << SearchManager::TYPE_DIRECTORY)
{
}

string ShareManager::Directory::getADCPath() const noexcept {
    if(!getParent())
        return '/' + name + '/';
    return getParent()->getADCPath() + name + '/';
}

string ShareManager::Directory::getFullName() const noexcept {
    if(!getParent())
        return getName() + '\\';
    return getParent()->getFullName() + getName() + '\\';
}

void ShareManager::Directory::addType(uint32_t type) noexcept {
    if(!hasType(type)) {
        fileTypes |= (1 << type);
        if(getParent())
            getParent()->addType(type);
    }
}

string ShareManager::Directory::getRealPath(const std::string& path) const {
    if(getParent()) {
        return getParent()->getRealPath(getName() + PATH_SEPARATOR_STR + path);
    } else {
        return ShareManager::getInstance()->findRealRoot(getName(), path);
    }
}

string ShareManager::findRealRoot(const string& virtualRoot, const string& virtualPath) const {
    for(auto& i : shares) {
        if(Util::stricmp(i.second, virtualRoot) == 0) {
            std::string name = i.first + virtualPath;
            dcdebug("Matching %s\n", name.c_str());
            if (File::getSize(name) != -1) //NOTE: see core 0.750
                return name;
        }
    }

    throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
}

int64_t ShareManager::Directory::getSize() const noexcept {
    int64_t tmp = size;
    for(auto& i : directories)
        tmp += i.second->getSize();
    return tmp;
}

string ShareManager::toVirtual(const TTHValue& tth) const {
    if(fileList.isBzList(tth)) {
        return Transfer::USER_LIST_NAME_BZ;
    } else if(fileList.isXmlList(tth)) {
        return Transfer::USER_LIST_NAME;
    }

    Lock l(cs);
    auto i = tthIndex.find(tth);
    if(i != tthIndex.end()) {
        return i->second->getADCPath();
    } else {
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    }
}

string ShareManager::toReal(const string& virtualFile) {
    Lock l(cs);
    if(virtualFile == "MyList.DcLst") {
        throw ShareException("NMDC-style lists no longer supported, please upgrade your client");
    } else if(virtualFile == Transfer::USER_LIST_NAME_BZ || virtualFile == Transfer::USER_LIST_NAME) {
        return fileList.ensure();
    }

    return findFile(virtualFile)->getRealPath();
}

StringList ShareManager::getRealPaths(const string& virtualPath) {
    if(virtualPath.empty())
        throw ShareException("empty virtual path");

    StringList ret;

    Lock l(cs);

    if(*(virtualPath.end() - 1) == '/') {
        // directory
        Directory::Ptr d = splitVirtual(virtualPath).first;

        // imitate Directory::getRealPath
        if(d->getParent()) {
            ret.push_back(d->getParent()->getRealPath(d->getName()));
        } else {
            for(auto& i : shares) {
                if(Util::stricmp(i.second, d->getName()) == 0) {
                    // remove the trailing path sep
                    if(FileFindIter(i.first.substr(0, i.first.size() - 1)) != FileFindIter()) {
                        ret.push_back(i.first);
                    }
                }
            }
        }

    } else {
        // file
        ret.push_back(toReal(virtualPath));
    }

    return ret;
}

TTHValue ShareManager::getTTH(const string& virtualFile) const {
    Lock l(cs);
    if(virtualFile == Transfer::USER_LIST_NAME_BZ) {
        return fileList.getBzRoot();
    } else if(virtualFile == Transfer::USER_LIST_NAME) {
        return fileList.getXmlRoot();
    }

    return findFile(virtualFile)->getTTH();
}

MemoryInputStream* ShareManager::getTree(const string& virtualFile) const {
    TigerTree tree;
    if(virtualFile.compare(0, 4, "TTH/") == 0) {
        if(!HashManager::getInstance()->getTree(TTHValue(virtualFile.substr(4)), tree))
            return 0;
    } else {
        try {
            TTHValue tth = getTTH(virtualFile);
            HashManager::getInstance()->getTree(tth, tree);
        } catch(const Exception&) {
            return 0;
        }
    }

    ByteVector buf = tree.getLeafData();
    return new MemoryInputStream(&buf[0], buf.size());
}

AdcCommand ShareManager::getFileInfo(const string& aFile) {
    if(aFile == Transfer::USER_LIST_NAME) {
        fileList.ensure();
        AdcCommand cmd(AdcCommand::CMD_RES);
        cmd.addParam("FN", aFile);
        cmd.addParam("SI", Util::toString(fileList.getXmlSize()));
        cmd.addParam("TR", fileList.getXmlRoot().toBase32());
        return cmd;
    } else if(aFile == Transfer::USER_LIST_NAME_BZ) {
        fileList.ensure();

        AdcCommand cmd(AdcCommand::CMD_RES);
        cmd.addParam("FN", aFile);
        cmd.addParam("SI", Util::toString(fileList.getBzSize()));
        cmd.addParam("TR", fileList.getBzRoot().toBase32());
        return cmd;
    }

    if(aFile.compare(0, 4, "TTH/") != 0)
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);

    TTHValue val(aFile.substr(4));
    Lock l(cs);
    auto i = tthIndex.find(val);
    if(i == tthIndex.end()) {
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    }

    const Directory::File& f = *i->second;
    AdcCommand cmd(AdcCommand::CMD_RES);
    cmd.addParam("FN", f.getADCPath());
    cmd.addParam("SI", Util::toString(f.getSize()));
    cmd.addParam("TR", f.getTTH().toBase32());
    return cmd;
}

pair<ShareManager::Directory::Ptr, string> ShareManager::splitVirtual(const string& virtualPath) const {
    if(virtualPath.empty() || virtualPath[0] != '/') {
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    }

    auto i = virtualPath.find('/', 1);
    if(i == string::npos || i == 1) {
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    }

    auto dmi = getByVirtual( virtualPath.substr(1, i - 1));
    if(dmi == directories.end()) {
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    }

    auto d = *dmi;

    auto j = i + 1;
    while((i = virtualPath.find('/', j)) != string::npos) {
        auto mi = d->directories.find(virtualPath.substr(j, i - j));
        j = i + 1;
        if(mi == d->directories.end())
            throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
        d = mi->second;
    }

    return make_pair(d, virtualPath.substr(j));
}

ShareManager::Directory::File::Set::const_iterator ShareManager::findFile(const string& virtualFile) const {
    if(virtualFile.compare(0, 4, "TTH/") == 0) {
        auto i = tthIndex.find(TTHValue(virtualFile.substr(4)));
        if(i == tthIndex.end()) {
            throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
        }
        return i->second;
    }

    auto v = splitVirtual(virtualFile);
    auto it = find_if(v.first->files.begin(), v.first->files.end(),
                      Directory::File::StringComp(v.second));
    if(it == v.first->files.end())
        throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
    return it;
}

string ShareManager::validateVirtual(const string& aVirt) const noexcept {
    string tmp = aVirt;
    string::size_type idx = 0;

    while( (idx = tmp.find_first_of("\\/"), idx) != string::npos) {
        tmp[idx] = '_';
    }
    return tmp;
}

bool ShareManager::hasVirtual(const string& virtualName) const noexcept {
    Lock l(cs);
    return getByVirtual(virtualName) != directories.end();
}

void ShareManager::load(SimpleXML& aXml) {
    Lock l(cs);

    aXml.resetCurrentChild();
    if(aXml.findChild("Share")) {
        aXml.stepIn();
        while(aXml.findChild("Directory")) {
            string realPath = aXml.getChildData();
            if(realPath.empty()) {
                continue;
            }
            // make sure realPath ends with a PATH_SEPARATOR
            if(realPath[realPath.size() - 1] != PATH_SEPARATOR) {
                realPath += PATH_SEPARATOR;
            }

            const string& virtualName = aXml.getChildAttrib("Virtual");
            string vName = validateVirtual(virtualName.empty() ? Util::getLastDir(realPath) : virtualName);
            shares.insert(std::make_pair(realPath, vName));
            if(getByVirtual(vName) == directories.end()) {
                directories.push_back(Directory::create(vName));
            }
        }
        try {
            aXml.stepOut();
        }
        catch(const Exception&) { }
    }
}

void ShareManager::save(SimpleXML& aXml) {
    Lock l(cs);

    aXml.addTag("Share");
    aXml.stepIn();
    for(auto& i: shares) {
        aXml.addTag("Directory", i.first);
        aXml.addChildAttrib("Virtual", i.second);
    }
    try {
        aXml.stepOut();
    }
    catch(const Exception&) { }
}

void ShareManager::addDirectory(const string& realPath, const string& virtualName) {
    if(realPath.empty() || virtualName.empty()) {
        throw ShareException(_("No directory specified"));
    }

    if (!checkHidden(realPath)) {
        throw ShareException(_("Directory is hidden"));
    }

    if(Util::stricmp(SETTING(TEMP_DOWNLOAD_DIRECTORY), realPath) == 0) {
        throw ShareException(_("The temporary download directory cannot be shared"));
    }
    list<string> removeMap;
    {
        Lock l(cs);

        for(auto& i: shares) {
            if(Util::strnicmp(realPath, i.first, i.first.length()) == 0) {
                // Trying to share an already shared directory
                //throw ShareException(_("Directory already shared"));
                removeMap.push_front(i.first);
            } else if(Util::strnicmp(realPath, i.first, realPath.length()) == 0) {
                // Trying to share a parent directory
                //throw ShareException(_("Remove all subdirectories before adding this one"));
                removeMap.push_front(i.first);
            }
        }
    }
    for(auto& i : removeMap) {
        removeDirectory(i);
    }

    HashManager::HashPauser pauser;

    auto dp = buildTree(realPath, Directory::Ptr());

    string vName = validateVirtual(virtualName);
    dp->setName(vName);

    {
        Lock l(cs);

        shares.insert(std::make_pair(realPath, vName));
        updateIndices(*merge(dp));

        setDirty();
    }
}

ShareManager::Directory::Ptr ShareManager::merge(const Directory::Ptr& directory) {
    for(auto& i : directories) {
        if(Util::stricmp(i->getName(), directory->getName()) == 0) {
            dcdebug("Merging directory %s\n", directory->getName().c_str());
            i->merge(directory);
            return i;
        }
    }

    dcdebug("Adding new directory %s\n", directory->getName().c_str());

    directories.push_back(directory);
    return directory;
}

void ShareManager::Directory::merge(const Directory::Ptr& source) {
    // merge directories
    for(auto& i: source->directories) {
        auto subSource = i.second;

        auto ti = directories.find(subSource->getName());
        if(ti == directories.end()) {
            if(findFile(subSource->getName()) != files.end()) {
                dcdebug("File named the same as directory");
            } else {
                // the directory doesn't exist; create it.
                directories.emplace(subSource->getName(), subSource);
                subSource->parent = this;
            }
        } else {
            // the directory was already existing; merge into it.
            auto subTarget = ti->second;
            subTarget->merge(subSource);
        }
    }

    // All subdirs either deleted or moved to target...
    source->directories.clear();

    // merge files
    for(auto& i: source->files) {
        if(findFile(i.getName()) == files.end()) {
            if(directories.find(i.getName()) != directories.end()) {
                dcdebug("Directory named the same as file");
            } else {
                auto added = files.insert(i);
                if(added.second) {
                    const_cast<File&>(*added.first).setParent(this);
                }
            }
        }
    }
}

void ShareManager::removeDirectory(const string& realPath) {
    if(realPath.empty())
        return;

    HashManager::getInstance()->stopHashing(realPath);

    Lock l(cs);

    auto i = shares.find(realPath);
    if(i == shares.end()) {
        return;
    }

    auto vName = i->second;
    for(auto j = directories.begin(); j != directories.end(); ) {
        if(Util::stricmp((*j)->getName(), vName) == 0) {
            directories.erase(j++);
        } else {
            ++j;
        }
    }

    shares.erase(i);

    HashManager::HashPauser pauser;

    // Readd all directories with the same vName
    for(i = shares.begin(); i != shares.end(); ++i) {
        if(Util::stricmp(i->second, vName) == 0 && checkHidden(i->first)) {
            auto dp = buildTree(i->first, 0);
            dp->setName(i->second);
            merge(dp);
        }
    }

    rebuildIndices();
    setDirty();
}

void ShareManager::renameDirectory(const string& realPath, const string& virtualName) {
    removeDirectory(realPath);
    addDirectory(realPath, virtualName);
}

ShareManager::DirList::const_iterator ShareManager::getByVirtual(const string& virtualName) const noexcept {
    for(auto i = directories.begin(); i != directories.end(); ++i) {
        if(Util::stricmp((*i)->getName(), virtualName) == 0) {
            return i;
        }
    }
    return directories.end();
}

int64_t ShareManager::getShareSize(const string& realPath) const noexcept {
    Lock l(cs);
    dcassert(!realPath.empty());
    auto i = shares.find(realPath);

    if(i != shares.end()) {
        auto j = getByVirtual(i->second);
        if(j != directories.end()) {
            return (*j)->getSize();
        }
    }
    return -1;
}

int64_t ShareManager::getShareSize() const noexcept {
    Lock l(cs);
    int64_t tmp = 0;
    for(auto& i: tthIndex) {
        tmp += i.second->getSize();
    }
    return tmp;
}

size_t ShareManager::getSharedFiles() const noexcept {
    Lock l(cs);
    return tthIndex.size();
}

ShareManager::Directory::Ptr ShareManager::buildTree(const string& aName, const Directory::Ptr& aParent) {
    return ShareTreeScan().build(aName, aParent);
}

//NOTE: freedcpp [+
#ifdef _WIN32
bool ShareManager::checkHidden(const string& aName) const {
    FileFindIter ff = FileFindIter(aName.substr(0, aName.size() - 1));

    if (ff != FileFindIter()) {
        return (BOOLSETTING(SHARE_HIDDEN) || !ff->isHidden());
    }

    return true;
}

#else // !_WIN32

bool ShareManager::checkHidden(const string& aName) const
{
    // check open a directory
    if (!(FileFindIter(aName) != FileFindIter()))
        return true;

    // check hidden directory
    bool hidden = false;
    string path = aName.substr(0, aName.size() - 1);
    string::size_type i = path.rfind(PATH_SEPARATOR);

    if (i != string::npos)
    {
        string dir = path.substr(i + 1);
        if (dir[0] == '.')
            hidden = true;
    }

    return (BOOLSETTING(SHARE_HIDDEN) || !hidden);
}
#endif // !_WIN32
//NOTE: freedcpp +]

void ShareManager::refresh(bool dirs /* = false */, bool aUpdate /* = true */, bool block /* = false */) noexcept {
    if(refreshing.exchange(true) == true) {
        LogManager::getInstance()->message(_("File list refresh in progress, please wait for it to finish before trying to refresh again"));
        return;
    }
    UploadManager::getInstance()->updateLimits();

    update = aUpdate;
    refreshDirs = dirs;
    join();
    bool cached = false;
    if(initial) {
        cached = fileList.loadCache();
        initial = false;
        // Cache already rebuilt the tree. A full disk walk here only slows first paint
        // and fights the UI for I/O. With auto-refresh on, mark lastFullUpdate = 0 so the
        // next Minute tick rescans; with it off, keep the one background walk.
        if(cached) {
            if(SETTING(AUTO_REFRESH_TIME) > 0) {
                refreshDirs = false;
                lastFullUpdate = 0;
            }
        }
    }
    try {
        setThreadPriority(Thread::LOW);
        start();
        if(block && !cached) {
            join();
        }
    } catch(const ThreadException& e) {
        LogManager::getInstance()->message(str(F_("File list refresh failed: %1%") % e.getError()));
    }
}

StringPairList ShareManager::getDirectories() const noexcept {
    Lock l(cs);
    StringPairList ret;
    for(auto& i: shares) {
        ret.emplace_back(i.second, i.first);
    }
    return ret;
}

int ShareManager::run() {
    StringPairList dirs = getDirectories();
    // Don't need to refresh if no directories are shared
    if(dirs.empty())
        refreshDirs = false;

    if(refreshDirs) {
        HashManager::HashPauser pauser;
        LogManager::getInstance()->message(_("File list refresh initiated"));

        lastFullUpdate = GET_TICK();

        DirList newDirs;
        for(auto& i: dirs) {
            if (checkHidden(i.second)) {
                auto dp = buildTree(i.second, Directory::Ptr());
                dp->setName(i.first);
                newDirs.emplace_back(dp);
            }
        }

        {
            Lock l(cs);
            directories.clear();

            for(auto& i: newDirs) {
                merge(i);
            }

            rebuildIndices();
        }
        refreshDirs = false;
        MediaInfoCache::getInstance()->save();

        LogManager::getInstance()->message(_("File list refresh finished"));
    }

    if(update) {
        ClientManager::getInstance()->infoUpdated();
    }
    refreshing = false;
#ifdef WITH_DHT
    dht::IndexManager* im = dht::IndexManager::getInstance();
    if(im && im->isTimeForPublishing())
        im->setNextPublishing();
#endif
    return 0;
}

void ShareManager::getBloom(ByteVector& v, size_t k, size_t m, size_t h) const {
    dcdebug("Creating bloom filter, k=%u, m=%u, h=%u\n",
            static_cast<unsigned int>(k), static_cast<unsigned int>(m), static_cast<unsigned int>(h));
    Lock l(cs);

    HashBloom bloom;
    bloom.reset(k, m, h);
    for(auto& i: tthIndex) {
        bloom.add(i.first);
    }
    bloom.copy_to(v);
}

SearchManager::TypeModes ShareManager::getType(const string& aFileName) const noexcept {
    return ShareFileType::classify(aFileName);
}

void ShareManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
    if (SETTING(AUTO_REFRESH_TIME) > 0) {
        // 0 means “rescan soon” after a cache-hit startup that skipped the walk.
        if(lastFullUpdate == 0 || lastFullUpdate + SETTING(AUTO_REFRESH_TIME) * 60 * 1000 <= tick) {
            refresh(true, true);
        }
    }
}

} // namespace dcpp
