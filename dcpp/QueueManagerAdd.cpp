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
#include "DownloadManager.h"
#include "File.h"
#include "HashManager.h"
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "StringTokenizer.h"

#if !defined(_WIN32) && !defined(PATH_MAX)
#if defined(__linux)
#include <sys/syslimits.h>
#elif defined(__GNU__)
#define PATH_MAX 4096
#endif
#endif

namespace dcpp {

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root)
{
    // Check if we're not downloading something already in our share
    if (BOOLSETTING(DONT_DL_ALREADY_SHARED))
    {
        if (ShareManager::getInstance()->isTTHShared(root))
        {
            throw QueueException(_("A file with the same hash already exists in your share"));
        }
    }
    // Check that target contains at least one directory...we don't want headless files...
    // Check that the file doesn't already exist...
    const string target = checkTarget(aTarget, aSize);

    // Check if it's a zero-byte file, if so, create and return...
    // Hashed magnets with size 0 still queue (size may arrive from the peer).
    if (aSize == 0 && !root)
    {
        if (!BOOLSETTING(SKIP_ZERO_BYTE))
        {
            File::ensureDirectory(target);
            File f(target, File::WRITE, File::CREATE);
        }
        return;
    }

    Lock l(cs);

    // This will be pretty slow on large queues...
    if (BOOLSETTING(DONT_DL_ALREADY_QUEUED))
    {
        auto ql = fileQueue.find(root);
        if (!ql.empty())
            throw QueueException(_("This file is already queued"));
    }

    QueueItem* q = fileQueue.find(target);

    if(q == NULL)
    {
        q = fileQueue.add(target, aSize, 0, QueueItem::DEFAULT/*QueueItem::Priority*/, Util::emptyString/*aTempTarget*/,
                          GET_TIME()/*time_t aAdded*/, root/*TTHValue& root*/);
        fire(QueueManagerListener::Added(), q);
    } else {
        if(q->getSize() != aSize)
        {
            throw QueueException(_("A file with a different size already exists in the queue"));
        }
        if(!(root == q->getTTH()))
        {
            throw QueueException(_("A file with different tth root already exists in the queue"));
        }
    }
}
// NOTE: freedcpp: end

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
                       int aFlags /* = 0 */, bool addBad /* = true */)
{
    bool wantConnection = true;

    // Check that we're not downloading from ourselves...
    if(aUser == ClientManager::getInstance()->getMe()) {
        throw QueueException(_("You're trying to download from yourself!"));
    }

    // Check if we're not downloading something already in our share
    if(BOOLSETTING(DONT_DL_ALREADY_SHARED)){
        if (ShareManager::getInstance()->isTTHShared(root)){
            throw QueueException(_("A file with the same hash already exists in your share"));
        }
    }

    string target;
    string tempTarget;
    if((aFlags & QueueItem::FLAG_USER_LIST) == QueueItem::FLAG_USER_LIST) {
        target = getListPath(aUser);
        tempTarget = aTarget;
    } else {
        target = checkTarget(aTarget, /*checkExistence*/ true);
    }

    // True empty files (no TTH): create locally. Hashed results may report size 0
    // (e.g. ShareIndex gaps) and must still enter the queue.
    if(aSize == 0 && !root) {
        if(!BOOLSETTING(SKIP_ZERO_BYTE)) {
            File::ensureDirectory(target);
            File f(target, File::WRITE, File::CREATE);
        }
        return;
    }

    {
        Lock l(cs);

        // This will be pretty slow on large queues...
        if(BOOLSETTING(DONT_DL_ALREADY_QUEUED) && !(aFlags & QueueItem::FLAG_USER_LIST)) {
            auto ql = fileQueue.find(root);
            if (!ql.empty()) {
                // Found one or more existing queue items, lets see if we can add the source to them
                bool sourceAdded = false;
                for(auto& i : ql) {
                    if(!i->isSource(aUser)) {
                        try {
                            wantConnection = addSource(i, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0);
                            sourceAdded = true;
                        } catch(...) { }
                    }
                }

                if(!sourceAdded) {
                    throw QueueException(_("This file is already queued"));
                }
                goto connect;
            }
        }

        QueueItem* q = fileQueue.find(target);
        if(q == NULL) {
            q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, tempTarget, GET_TIME(), root);
            fire(QueueManagerListener::Added(), q);
        } else {
            if(q->getSize() != aSize) {
                throw QueueException(_("A file with a different size already exists in the queue"));
            }
            if(!(root == q->getTTH())) {
                throw QueueException(_("A file with a different TTH root already exists in the queue"));
            }

            if(q->isFinished()) {
                throw QueueException(_("This file has already finished downloading"));
            }

            q->setFlag(aFlags);
        }

        wantConnection = addSource(q, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0);
    }

connect:
    if(wantConnection && aUser.user->isOnline())
        ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::readd(const string& target, const HintedUser& aUser) {
    bool wantConnection = false;
    {
        Lock l(cs);
        QueueItem* q = fileQueue.find(target);
        if(q && q->isBadSource(aUser)) {
            wantConnection = addSource(q, aUser, QueueItem::Source::FLAG_MASK);
        }
    }
    if(wantConnection && aUser.user->isOnline())
        ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

} // namespace dcpp
