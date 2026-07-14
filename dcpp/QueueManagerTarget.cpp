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

#include "ConnectionManager.h"
#include "File.h"
#include "LogManager.h"
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

void QueueManager::setDirty() {
    if(!dirty) {
        dirty = true;
        lastSave = GET_TICK();
    }
}

string QueueManager::checkTarget(const string& aTarget, bool checkExistence) {
#ifdef _WIN32
    if(aTarget.length() > MAX_PATH) {
        throw QueueException(_("Target filename too long"));
    }
    // Check that target starts with a drive or is an UNC path
    if( (aTarget[1] != ':' || aTarget[2] != '\\') &&
            (aTarget[0] != '\\' && aTarget[1] != '\\') ) {
        throw QueueException(_("Invalid target file (missing directory, check default download directory setting)"));
    }
#else
    if(aTarget.length() > PATH_MAX) {
        throw QueueException(_("Target filename too long"));
    }
    // Check that target contains at least one directory...we don't want headless files...
    if(aTarget[0] != '/') {
        throw QueueException(_("Invalid target file (missing directory, check default download directory setting)"));
    }
#endif

    string target = Util::validateFileName(aTarget);

    // Check that the file doesn't already exist...
    if(checkExistence && File::getSize(target) != -1) {
        throw FileException(_("File already exists at the target location"));
    }
    return target;
}

QueueItem::Priority QueueManager::hasDownload(const UserPtr& aUser) noexcept {
    Lock l(cs);
    QueueItem* qi = userQueue.getNext(aUser, QueueItem::LOWEST);
    if(!qi) {
        return QueueItem::PAUSED;
    }
    return qi->getPriority();
}

int64_t QueueManager::getPos(const string& target) noexcept {
    Lock l(cs);
    QueueItem* qi = fileQueue.find(target);
    if(qi) {
        return qi->getDownloadedBytes();
    }
    return -1;
}

int64_t QueueManager::getSize(const string& target) noexcept {
    Lock l(cs);
    QueueItem* qi = fileQueue.find(target);
    if(qi) {
        return qi->getSize();
    }
    return -1;
}

void QueueManager::getSizeInfo(int64_t& size, int64_t& pos, const string& target) noexcept {
    Lock l(cs);
    QueueItem* qi = fileQueue.find(target);
    if(qi) {
        size = qi->getSize();
        pos = qi->getDownloadedBytes();
    } else {
        size = -1;
    }
}

void QueueManager::move(const string& aSource, const string& aTarget) noexcept {
    string target = Util::validateFileName(aTarget);
    if(aSource == target)
        return;

    bool delSource = false;
    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();

    Lock l(cs);
    QueueItem* qs = fileQueue.find(aSource);
    if(qs) {
        // Don't move running downloads
        if(qs->isRunning()) {
            return;
        }
        // Don't move file lists
        if(qs->isSet(QueueItem::FLAG_USER_LIST))
            return;

        // Let's see if the target exists...then things get complicated...
        QueueItem* qt = fileQueue.find(target);
        if(qt == NULL || Util::stricmp(aSource, target) == 0) {
            // Good, update the target and move in the queue...
            fileQueue.move(qs, target);
            fire(QueueManagerListener::Moved(), qs, aSource);
            setDirty();
        } else {
            // Don't move to target of different size
            if(qs->getSize() != qt->getSize() || qs->getTTH() != qt->getTTH())
                return;

            for(auto& i: qs->getSources()) {
                try {
                    addSource(qt, i.getUser(), QueueItem::Source::FLAG_MASK, &queued);
                } catch(const Exception&) {
                }
            }
            delSource = true;
        }
    }

    if(delSource) {
        remove(aSource);
    }
}

StringList QueueManager::getTargets(const TTHValue& tth) {
    Lock l(cs);
    auto ql = fileQueue.find(tth);
    StringList sl;
    for(auto& i: ql) {
        sl.push_back(i->getTarget());
    }
    return sl;
}

} // namespace dcpp
