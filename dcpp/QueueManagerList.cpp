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
#include "ConnectionManagerPeerMatch.h"
#include "PeerConnectLog.h"

namespace dcpp {

void QueueManager::addList(const HintedUser& aUser, int aFlags, const string& aInitialDir /* = Util::emptyString */) {
    const bool autoList = aFlags == 0
            || ((aFlags & QueueItem::FLAG_DIRECTORY_DOWNLOAD)
                && !(aFlags & (QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST)));
    if(aUser.user && aUser.user->isSet(User::VIRUS_INFECTED) && autoList) {
        PeerConnectLog::skip(ClientManager::getInstance()->getNickOrCid(aUser), aUser.hint,
                             _("user marked as virus-infected"));
        throw QueueException(_("User is marked as virus-infected"));
    }

    OnlineUser* ou = 0;
    if(!aUser.hint.empty())
        ou = ClientManager::getInstance()->findOnlineUser(aUser.user->getCID(), aUser.hint, true);
    else
        ou = ClientManager::getInstance()->findOnlineUser(aUser, false);

    if(!ou) {
        PeerConnectLog::skip(ClientManager::getInstance()->getNickOrCid(aUser), aUser.hint, _("user not online on hub"));
        throw QueueException(_("User is not online on this hub"));
    }

    if(!(aFlags & QueueItem::FLAG_PARTIAL_LIST)) {
        Lock l(cs);
        QueueItem* qi = fileQueue.find(getListPath(aUser));
        if(qi && qi->isSet(QueueItem::FLAG_USER_LIST) && !qi->isFinished()) {
            qi->setFlag(aFlags);
            if(!aInitialDir.empty())
                qi->setTempTarget(aInitialDir);
            return;
        }
        for(const auto& i: fileQueue.getQueue()) {
            qi = i.second;
            if(!qi->isSet(QueueItem::FLAG_USER_LIST) || qi->isFinished() || qi->getSources().empty())
                continue;
            if(!ConnectionManagerPeerMatch::samePeer(aUser, qi->getSources()[0].getUser()))
                continue;
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

size_t QueueManager::countQueuedLists() noexcept {
    Lock l(cs);
    size_t n = 0;
    for(const auto& i: fileQueue.getQueue()) {
        if(i.second->isSet(QueueItem::FLAG_USER_LIST) && !i.second->isFinished())
            ++n;
    }
    return n;
}

void QueueManager::removeUserLists() noexcept {
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
        }
    }
}

} // namespace dcpp
