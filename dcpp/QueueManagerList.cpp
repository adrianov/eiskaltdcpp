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
#include "DirectoryListing.h"
#include "ListCache.h"
#include "LogManager.h"
#include "PeerConnectLog.h"

#include <unordered_set>

namespace dcpp {

void QueueManager::purgeOtherListQueues(const HintedUser& aUser) {
    StringList nicks = ClientManager::getInstance()->getNicks(aUser);
    if(nicks.empty())
        return;

    std::unordered_set<CID> sameNick;
    ClientManager::getInstance()->cidsForNick(nicks[0], sameNick);
    StringList stale;

    {
        Lock l(cs);
        for(auto& i: fileQueue.getQueue()) {
            QueueItem* qi = i.second;
            if(!qi->isSet(QueueItem::FLAG_USER_LIST) || qi->isFinished() || qi->getSources().empty())
                continue;

            const HintedUser& src = qi->getSources()[0].getUser();
            if(src.user == aUser.user)
                continue;

            if(sameNick.count(src.user->getCID()))
                stale.push_back(qi->getTarget());
        }
    }

    for(auto& target: stale)
        remove(target);
}

void QueueManager::addList(const HintedUser& aUser, int aFlags, const string& aInitialDir /* = Util::emptyString */) {
    OnlineUser* ou = 0;
    if(!aUser.hint.empty())
        ou = ClientManager::getInstance()->findOnlineUser(aUser.user->getCID(), aUser.hint, true);
    else
        ou = ClientManager::getInstance()->findOnlineUser(aUser, false);

    if(!ou) {
        PeerConnectLog::skip(ClientManager::getInstance()->getNickOrCid(aUser), aUser.hint, _("user not online on hub"));
        throw QueueException(_("User is not online on this hub"));
    }

    // Merge browse / match / directory flags onto an in-flight silent list.
    if(!(aFlags & QueueItem::FLAG_PARTIAL_LIST)) {
        Lock l(cs);
        QueueItem* qi = fileQueue.find(getListPath(aUser));
        if(qi && qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isFinished()) {
            qi->setFlag(aFlags);
            if(!aInitialDir.empty())
                qi->setTempTarget(aInitialDir);
            return;
        }
    }

    purgeOtherListQueues(aUser);

    if(!(aFlags & QueueItem::FLAG_PARTIAL_LIST) && tryUseCachedList(aUser, aFlags, aInitialDir))
        return;
    add(aInitialDir, -1, TTHValue(), aUser, QueueItem::FLAG_USER_LIST | aFlags);
}

bool QueueManager::hasListQueued(const HintedUser& user) noexcept {
    if(!user.user)
        return false;
    Lock l(cs);
    QueueItem* qi = fileQueue.find(getListPath(user));
    return qi && qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isFinished();
}

void QueueManager::removeUserLists() noexcept {
    // Match by flag or FileLists/ path: Queue.xml does not persist FLAG_USER_LIST,
    // so reloaded list downloads look like normal files under that directory.
    const string listPath = Util::getListPath();
    StringList targets;
    {
        Lock l(cs);
        for(const auto& i: fileQueue.getQueue()) {
            const string& target = *i.first;
            if(i.second->isSet(QueueItem::FLAG_USER_LIST)
                    || Util::strnicmp(target.c_str(), listPath.c_str(), listPath.size()) == 0)
                targets.push_back(target);
        }
    }
    for(const auto& t: targets)
        remove(t);
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
        PeerConnectLog::cachedList(aUser, listFile);
        fire(QueueManagerListener::ListFromCache(), aUser, listFile, aInitialDir);
        return true;
    }

    if(processFlags == 0) {
        PeerConnectLog::cachedList(aUser, listFile);
        fire(QueueManagerListener::ListCached(), aUser, listFile);
    }
    return true;
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

} // namespace dcpp
