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
#include "DirectoryListing.h"

namespace dcpp {

DirectoryListing::Directory::~Directory() {
    std::for_each(directories.begin(), directories.end(), DeleteFunction());
    std::for_each(files.begin(), files.end(), DeleteFunction());
}

void DirectoryListing::Directory::pruneEmptyDirs() {
    for(auto i = directories.begin(); i != directories.end();) {
        auto d = *i;
        d->pruneEmptyDirs();
        if(d->files.empty() && d->directories.empty() && d->getComplete()) {
            i = directories.erase(i);
            delete d;
        } else {
            ++i;
        }
    }
}

void DirectoryListing::Directory::filterList(DirectoryListing& dirList) {
    DirectoryListing::Directory* d = dirList.getRoot();

    TTHSet l;
    d->getHashList(l);
    filterList(l);
}

void DirectoryListing::Directory::filterList(DirectoryListing::Directory::TTHSet& l) {
    for(auto i = directories.begin(); i != directories.end();) {
        auto d = *i;

        d->filterList(l);

        if(d->directories.empty() && d->files.empty()) {
            i = directories.erase(i);
            delete d;
        } else {
            ++i;
        }
    }

    for(auto i = files.begin(); i != files.end();) {
        auto f = *i;
        if(l.find(f->getTTH()) != l.end()) {
            i = files.erase(i);
            delete f;
        } else {
            ++i;
        }
    }
}

void DirectoryListing::Directory::getHashList(DirectoryListing::Directory::TTHSet& l) {
    for(auto i: directories) i->getHashList(l);
    for(auto i: files) l.insert(i->getTTH());
}

int64_t DirectoryListing::Directory::getTotalSize(bool adl) {
    int64_t x = getSize();
    for(auto i: directories) {
        if(!(adl && i->getAdls()))
            x += i->getTotalSize(adls);
    }
    return x;
}

size_t DirectoryListing::Directory::getTotalFileCount(bool adl) {
    size_t x = getFileCount();
    for(auto i: directories) {
        if(!(adl && i->getAdls()))
            x += i->getTotalFileCount(adls);
    }
    return x;
}

} // namespace dcpp
