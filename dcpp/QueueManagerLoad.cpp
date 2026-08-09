/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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
#include "LogManager.h"
#include "Segment.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "queue/LocalMatch.h"

namespace dcpp {

class QueueLoader : public SimpleXMLReader::CallBack {
public:
    QueueLoader() : cur(nullptr), inDownloads(false) { }
    void startTag(const string& name, StringPairList& attribs, bool simple);
    void endTag(const string& name);

private:
    void loadDownload(StringPairList& attribs, bool simple);
    void loadSegment(StringPairList& attribs);
    void loadSource(StringPairList& attribs);
    void pauseIfPathMissing();

    string target;
    QueueItem* cur;
    bool inDownloads;
};

void QueueManager::loadQueue() noexcept {
    try {
        QueueLoader l;
        Util::migrate(getQueueFile());

        File f(getQueueFile(), File::READ, File::OPEN);
        SimpleXMLReader(&l).parse(f);
        dirty = false;
    } catch(const Exception&) {
        // ...
    }
    // Drop confirmed local copies only; hashing unknown TTHs waits for the
    // Minute sweep so cold start skips a full-queue hash pass.
    localMatch->sweep(false);
}

static const string sDownloads = "Downloads";
static const string sDownload = "Download";
static const string sTempTarget = "TempTarget";
static const string sTarget = "Target";
static const string sSize = "Size";
static const string sDownloaded = "Downloaded";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sAdded = "Added";
static const string sTTH = "TTH";
static const string sCID = "CID";
static const string sHubHint = "Hub";
static const string sSegment = "Segment";
static const string sStart = "Start";

void QueueLoader::loadDownload(StringPairList& attribs, bool simple) {
    QueueManager* qm = QueueManager::getInstance();
    int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
    if(size == 0)
        return;
    try {
        // Local matches are removed after load via LocalMatch::sweep().
        target = QueueManager::checkTarget(getAttrib(attribs, sTarget, 0), false);
        if(target.empty())
            return;
    } catch(const Exception&) {
        return;
    }

    const string& tthRoot = getAttrib(attribs, sTTH, 5);
    if(tthRoot.empty())
        return;

    QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
    time_t added = static_cast<time_t>(Util::toInt(getAttrib(attribs, sAdded, 4)));
    if(added == 0)
        added = GET_TIME();

    string tempTarget = getAttrib(attribs, sTempTarget, 5);
    int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 5));
    if(downloaded > size || downloaded < 0)
        downloaded = 0;

    QueueItem* qi = qm->fileQueue.find(target);
    if(!qi) {
        qi = qm->fileQueue.add(target, size, 0, p, tempTarget, added, TTHValue(tthRoot));
        if(downloaded > 0)
            qi->addSegment(Segment(0, downloaded));
        qm->fire(QueueManagerListener::Added(), qi);
    }
    if(!simple)
        cur = qi;
}

void QueueLoader::loadSegment(StringPairList& attribs) {
    int64_t start = Util::toInt64(getAttrib(attribs, sStart, 0));
    int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
    if(size > 0 && start >= 0 && (start + size) <= cur->getSize())
        cur->addSegment(Segment(start, size));
}

void QueueLoader::pauseIfPathMissing() {
    if(Util::fileExists(Util::getFilePath(cur->getTarget())) || !BOOLSETTING(CHECK_TARGETS_PATHS_ON_START))
        return;
    QueueManager::getInstance()->setPriority(cur->getTarget(), QueueItem::PAUSED);
    LogManager::getInstance()->message(str(F_("Target path for this item is not available: %1%; pause this queue item.")
            % Util::addBrackets(cur->getTarget())));
}

void QueueLoader::loadSource(StringPairList& attribs) {
    const string& cid = getAttrib(attribs, sCID, 0);
    if(cid.length() != 39)
        return;

    QueueManager* qm = QueueManager::getInstance();
    UserPtr user = ClientManager::getInstance()->getUser(CID(cid));
    try {
        HintedUser hintedUser(user, getAttrib(attribs, sHubHint, 1));
        if(qm->addSource(cur, hintedUser, 0) && user->isOnline())
            ConnectionManager::getInstance()->getDownloadConnection(hintedUser);
    } catch(const Exception&) {
    }
}

void QueueLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
    if(!inDownloads && name == sDownloads) {
        inDownloads = true;
        return;
    }
    if(!inDownloads)
        return;

    if(cur == nullptr && name == sDownload) {
        loadDownload(attribs, simple);
        return;
    }
    if(!cur)
        return;

    if(name == sSegment)
        loadSegment(attribs);
    else if(name == sSource)
        loadSource(attribs);
    else
        pauseIfPathMissing();
}

void QueueLoader::endTag(const string& name) {
    if(!inDownloads)
        return;
    if(name == sDownload)
        cur = nullptr;
    else if(name == sDownloads)
        inDownloads = false;
}

} // namespace dcpp
