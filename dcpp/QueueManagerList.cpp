/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "DirectoryListing.h"
#include "ListCache.h"
#include "LogManager.h"

namespace dcpp {

void QueueManager::addList(const HintedUser& aUser, int aFlags, const string& aInitialDir /* = Util::emptyString */) {
    if(!(aFlags & QueueItem::FLAG_PARTIAL_LIST) && tryUseCachedList(aUser, aFlags, aInitialDir))
        return;
    add(aInitialDir, -1, TTHValue(), aUser, QueueItem::FLAG_USER_LIST | aFlags);
}

bool QueueManager::tryUseCachedList(const HintedUser& aUser, int aFlags, const string& aInitialDir) {
    const string listBase = getListPath(aUser);
    if(!ListCache::matchesUserShare(aUser, listBase))
        return false;

    const string listFile = ListCache::findListFile(listBase);
    const int processFlags = aFlags & (QueueItem::FLAG_DIRECTORY_DOWNLOAD | QueueItem::FLAG_MATCH_QUEUE);
    if(processFlags)
        processList(listFile, aUser, processFlags);

    if(aFlags & QueueItem::FLAG_CLIENT_VIEW) {
        fire(QueueManagerListener::ListFromCache(), aUser, listFile, aInitialDir);
        return true;
    }

    return processFlags != 0;
}

string QueueManager::getListPath(const HintedUser& user) {
    StringList nicks = ClientManager::getInstance()->getNicks(user);
    string nick = nicks.empty() ? Util::emptyString : Util::cleanPathChars(nicks[0]) + ".";
    return checkTarget(Util::getListPath() + nick + user.user->getCID().toBase32(), /*checkExistence*/ false);
}

void QueueManager::addDirectory(const string& aDir, const HintedUser& aUser, const string& aTarget, QueueItem::Priority p /* = QueueItem::DEFAULT */) noexcept {
    bool needList;
    {
        Lock l(cs);

        auto dp = directories.equal_range(aUser);

        for(auto i = dp.first; i != dp.second; ++i) {
            if(Util::stricmp(aDir.c_str(), i->second->getName().c_str()) == 0)
                return;
        }

        directories.emplace(aUser, new DirectoryItem(aUser, aDir, aTarget, p));
        needList = (dp.first == dp.second);
        setDirty();
    }

    if(needList) {
        try {
            addList(aUser, QueueItem::FLAG_DIRECTORY_DOWNLOAD);
        } catch(const Exception&) {
            // Ignore, we don't really care...
        }
    }
}

typedef unordered_map<TTHValue, const DirectoryListing::File*> TTHMap;

namespace {

void buildMap(const DirectoryListing::Directory* dir, TTHMap& tthMap) noexcept {
    std::for_each(dir->directories.cbegin(), dir->directories.cend(), [&](DirectoryListing::Directory* d) {
        if(!d->getAdls())
            buildMap(d, tthMap);
    });

    std::for_each(dir->files.cbegin(), dir->files.cend(), [&](DirectoryListing::File* f) {
        tthMap.emplace(f->getTTH(), f);
    });
}

class ListMatcher : public Thread {
public:
    ListMatcher(const StringList& files_) : files(files_) { }
    int run() {
        for(auto i = files.cbegin(); i != files.cend(); ++i) {
            UserPtr u = DirectoryListing::getUserFromFilename(*i);
            if(!u)
                continue;

            HintedUser user(u, Util::emptyString);
            DirectoryListing dl(user);
            try {
                dl.loadFile(*i);
                LogManager::getInstance()->message(str(F_("%1% : Matched %2% files") % Util::toString(ClientManager::getInstance()->getNicks(user)) % QueueManager::getInstance()->matchListing(dl)));
            } catch(const Exception&) { }
        }
        delete this;
        return 0;
    }
private:
    StringList files;
};

} // namespace

int QueueManager::matchListing(const DirectoryListing& dl) noexcept {
    int matches = 0;

    {
        Lock l(cs);
        TTHMap tthMap;
        buildMap(dl.getRoot(), tthMap);

        for(auto& i: fileQueue.getQueue()) {
            auto qi = i.second;
            if(qi->isFinished())
                continue;
            if(qi->isSet(QueueItem::FLAG_USER_LIST))
                continue;
            auto j = tthMap.find(qi->getTTH());
            if(j != tthMap.end() && j->second->getSize() == qi->getSize()) {
                try {
                    addSource(qi, dl.getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE);
                } catch(...) {
                    // Ignore...
                }
                matches++;
            }
        }
    }
    if(matches > 0)
        ConnectionManager::getInstance()->getDownloadConnection(dl.getUser());
    return matches;
}

void QueueManager::processList(const string& name, const HintedUser& user, int flags) {
    DirectoryListing dirList(user);
    try {
        dirList.loadFile(name);
    } catch(const Exception&) {
        LogManager::getInstance()->message(str(F_("Unable to open filelist: %1%") % Util::addBrackets(name)));
        return;
    }

    if(flags & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
        DirectoryItem::List dl;
        {
            Lock l(cs);
            auto dp = directories.equal_range(user);
            for(auto i = dp.first; i != dp.second; ++i) {
                dl.push_back(i->second);
            }
            directories.erase(user);
        }

        for(auto di: dl) {
            dirList.download(di->getName(), di->getTarget(), false);
            delete di;
        }
    }
    if(flags & QueueItem::FLAG_MATCH_QUEUE) {
        size_t files = matchListing(dirList);
        LogManager::getInstance()->message(str(FN_("%1%: Matched %2% file", "%1%: Matched %2% files", files) %
                                               Util::toString(ClientManager::getInstance()->getNicks(user)) % files));
    }
}

void QueueManager::matchAllListings() {
    ListMatcher* matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
    try {
        matcher->start();
    } catch(const ThreadException&) {
        ///@todo add error message
        delete matcher;
    }
}

} // namespace dcpp
