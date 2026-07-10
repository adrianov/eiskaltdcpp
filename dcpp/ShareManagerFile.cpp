/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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

#include "File.h"
#include "HashManager.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "Text.h"
#include "Util.h"

#ifdef WITH_DHT
#include "dht/IndexManager.h"
#endif

namespace dcpp {

void ShareManager::updateIndices(Directory& dir) {
    bloom.add(Text::toLower(dir.getName()));

    for(auto& i : dir.directories) {
        updateIndices(*i.second);
    }

    dir.size = 0;

    for(auto i = dir.files.begin(); i != dir.files.end(); ++i) {
        updateIndices(dir, i);
    }
}

void ShareManager::rebuildIndices() {
    tthIndex.clear();
    bloom.clear();

    for(auto& i: directories) {
        updateIndices(*i);
    }
}

void ShareManager::updateIndices(Directory& dir, const decltype(std::declval<Directory>().files.begin())& i) {
    const Directory::File& f = *i;

    auto j = tthIndex.find(f.getTTH());
    if(j == tthIndex.end()) {
        dir.size+=f.getSize();
    } else {
        if(!SETTING(LIST_DUPES)) {
            try {
                LogManager::getInstance()->message(str(F_("Duplicate file will not be shared: %1% (Size: %2% B) Dupe matched against: %3%")
                                                       % Util::addBrackets(dir.getRealPath(f.getName())) % Util::toString(f.getSize()) % Util::addBrackets(j->second->getParent()->getRealPath(j->second->getName()))));
                dir.files.erase(i);
            } catch (const ShareException&) {
            }
            return;
        }
    }

    dir.addType(getType(f.getName()));

    tthIndex.emplace(f.getTTH(), i);
    bloom.add(Text::toLower(f.getName()));
#ifdef WITH_DHT
    dht::IndexManager* im = dht::IndexManager::getInstance();
    if(im && im->isTimeForPublishing())
        im->publishFile(f.getTTH(), f.getSize());
#endif
}

ShareManager::Directory::Ptr ShareManager::getDirectory(const string& fname) {
    for(auto& mi : shares) {
        if(Util::strnicmp(fname, mi.first, mi.first.length()) == 0) {
            Directory::Ptr d;
            for(auto& i: directories) {
                if(Util::stricmp(i->getName(), mi.second) == 0) {
                    d = i;
                }
            }

            if(!d) {
                return Directory::Ptr();
            }

            string::size_type i;
            string::size_type j = mi.first.length();
            while((i = fname.find(PATH_SEPARATOR, j)) != string::npos) {
                auto dmi = d->directories.find(fname.substr(j, i-j));
                j = i + 1;
                if(dmi == d->directories.end())
                    return Directory::Ptr();
                d = dmi->second;
            }
            return d;
        }
    }
    return Directory::Ptr();
}

void ShareManager::removeFile(const string& realPath) noexcept {
    if(realPath.empty())
        return;

    HashManager::getInstance()->stopHashing(realPath);

    Lock l(cs);
    Directory::Ptr d = getDirectory(realPath);
    if(!d)
        return;

    auto i = d->findFile(Util::getFileName(realPath));
    if(i == d->files.end())
        return;

    auto tthIt = tthIndex.find(i->getTTH());
    if(tthIt != tthIndex.end() && tthIt->second == i) {
        d->size -= i->getSize();
        tthIndex.erase(tthIt);
    }

    d->files.erase(i);
    setDirty();
    forceXmlRefresh = true;
}

void ShareManager::on(QueueManagerListener::FileMoved, const string& realPath) noexcept {
    if(BOOLSETTING(ADD_FINISHED_INSTANTLY)) {
        // Check if finished download is supposed to be shared
        Lock l(cs);
        for(auto& i: shares) {
            if(Util::strnicmp(i.first, realPath, i.first.size()) == 0 && realPath[i.first.size() - 1] == PATH_SEPARATOR) {
                try {
                    // Schedule for hashing, it'll be added automatically later on...
                    HashManager::getInstance()->checkTTH(realPath, File::getSize(realPath), 0);
                } catch(const Exception&) {
                    // Not a vital feature...
                }
                break;
            }
        }
    }
}

void ShareManager::on(HashManagerListener::TTHDone, const string& realPath, const TTHValue& root) noexcept {
    Lock l(cs);
    Directory::Ptr d = getDirectory(realPath);
    if(d) {
        auto i = d->findFile(Util::getFileName(realPath));
        if(i != d->files.end()) {
            if(root != i->getTTH())
                tthIndex.erase(i->getTTH());
            // Get rid of false constness...
            auto f = const_cast<Directory::File*>(&(*i));
            f->setTTH(root);
            tthIndex.emplace(f->getTTH(), i);
        } else {
            string name = Util::getFileName(realPath);
            int64_t size = File::getSize(realPath);
            auto it = d->files.insert(Directory::File(name, size, d, root)).first;
            updateIndices(*d, it);
        }
        setDirty();
        forceXmlRefresh = true;
    }
}

} // namespace dcpp
