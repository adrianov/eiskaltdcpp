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

#include "ClientManager.h"
#include "QueueManager.h"
#include "ShareManager.h"

namespace dcpp {

DirectoryListing::DirectoryListing(const HintedUser& aUser) :
    user(aUser),
    root(new Directory(NULL, Util::emptyString, false, false))
{
}

DirectoryListing::~DirectoryListing() {
    delete root;
}

UserPtr DirectoryListing::getUserFromFilename(const string& fileName) {
    // General file list name format: [username].[CID].[xml|xml.bz2]

    string name = Util::getFileName(fileName);

    // Strip off any extensions
    if(Util::stricmp(name.c_str() + name.length() - 4, ".bz2") == 0) {
        name.erase(name.length() - 4);
    }

    if(Util::stricmp(name.c_str() + name.length() - 4, ".xml") == 0) {
        name.erase(name.length() - 4);
    }

    // Find CID
    string::size_type i = name.rfind('.');
    if(i == string::npos) {
        return UserPtr();
    }

    size_t n = name.length() - (i + 1);
    // CID's always 39 chars long...
    if(n != 39)
        return UserPtr();

    CID cid(name.substr(i + 1));
    if(!cid)
        return UserPtr();

    return ClientManager::getInstance()->getUser(cid);
}

string DirectoryListing::getPath(const Directory* d) const {
    if(d == root)
        return "";

    string dir;
    dir.reserve(128);
    dir.append(d->getName());
    dir.append(1, '\\');

    Directory* cur = d->getParent();
    while(cur!=root) {
        dir.insert(0, cur->getName() + '\\');
        cur = cur->getParent();
    }
    return dir;
}

StringList DirectoryListing::getLocalPaths(const File* f) const {
    try {
        return ShareManager::getInstance()->getRealPaths(Util::toAdcFile(getPath(f) + f->getName()));
    } catch(const ShareException&) {
        return StringList();
    }
}

StringList DirectoryListing::getLocalPaths(const Directory* d) const {
    try {
        return ShareManager::getInstance()->getRealPaths(Util::toAdcFile(getPath(d)));
    } catch(const ShareException&) {
        return StringList();
    }
}

void DirectoryListing::download(Directory* aDir, const string& aTarget, bool highPrio) {
    string target = (aDir == getRoot()) ? aTarget : aTarget + aDir->getName() + PATH_SEPARATOR;
    for(auto& j: aDir->directories) {
        download(j, target, highPrio);
    }
    for(auto file: aDir->files) {
        try {
            download(file, target + file->getName(), false, highPrio);
        } catch(const QueueException&) {
        } catch(const FileException&) {
        }
    }
}

void DirectoryListing::download(const string& aDir, const string& aTarget, bool highPrio) {
    dcassert(aDir.size() > 2);
    dcassert(aDir[aDir.size() - 1] == '\\');
    Directory* d = find(aDir, getRoot());
    if(d)
        download(d, aTarget, highPrio);
}

void DirectoryListing::download(File* aFile, const string& aTarget, bool view, bool highPrio) {
    int flags = (view ? (QueueItem::FLAG_TEXT | QueueItem::FLAG_CLIENT_VIEW) : 0);

    QueueManager::getInstance()->add(aTarget, aFile->getSize(), aFile->getTTH(), getUser(), flags);

    if(highPrio)
        QueueManager::getInstance()->setPriority(aTarget, QueueItem::HIGHEST);
}

DirectoryListing::Directory* DirectoryListing::find(const string& aName, Directory* current) const {
    auto end = aName.find('\\');
    dcassert(end != string::npos);
    auto name = aName.substr(0, end);

    auto i = std::find(current->directories.cbegin(), current->directories.cend(), name);
    if(i != current->directories.cend()) {
        if(end == (aName.size() - 1))
            return *i;
        else
            return find(aName.substr(end + 1), *i);
    }
    return nullptr;
}

} // namespace dcpp
