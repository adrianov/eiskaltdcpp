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

#include "Download.h"
#include "HashManager.h"
#include "SettingsManager.h"
#include "Transfer.h"
#include "UserConnection.h"

namespace dcpp {

Download* QueueManager::getDownload(UserConnection& aSource, bool supportsTrees) noexcept {
    Lock l(cs);

    UserPtr& u = aSource.getUser();
    dcdebug("Getting download for %s...", u->getCID().toBase32().c_str());

    QueueItem* q = userQueue.getNext(u, QueueItem::LOWEST, aSource.getChunkSize());

    if(!q) {
        dcdebug("none\n");
        return 0;
    }

    // Check that the file we will be downloading to exists
    if(q->getDownloadedBytes() > 0) {
        int64_t tempSize = File::getSize(q->getTempTarget());
        if(tempSize != q->getSize()) {
            // <= 0.706 added ".antifrag" to temporary download files if antifrag was enabled...
            // 0.705 added ".antifrag" even if antifrag was disabled
            std::string antifrag = q->getTempTarget() + ".antifrag";
            if(File::getSize(antifrag) > 0) {
                File::renameFile(antifrag, q->getTempTarget());
                tempSize = File::getSize(q->getTempTarget());
            }
            if(tempSize != q->getSize()) {
                if(tempSize > 0 && tempSize < q->getSize()) {
                    // Probably started with <=0.699 or with 0.705 without antifrag enabled...
                    try {
                        File(q->getTempTarget(), File::WRITE, File::OPEN).setSize(q->getSize());
                    } catch(const FileException&) { }
                } else {
                    // Temp target gone?
                    q->resetDownloaded();
                }
            }
        }
    }
    Download* d = new Download(aSource, *q, q->isSet(QueueItem::FLAG_PARTIAL_LIST) ? q->getTempTarget() : q->getTarget(), supportsTrees);

    userQueue.addDownload(q, d);

    fire(QueueManagerListener::StatusUpdated(), q);
    dcdebug("found %s\n", q->getTarget().c_str());
    return d;
}

bool QueueManager::isChunkDownloaded(const TTHValue& tth, int64_t startPos, int64_t& bytes,
        string& tempTarget, int64_t& size) {
    Lock l(cs);
    auto ql = fileQueue.find(tth);
    if(ql.empty())
        return false;
    QueueItem* qi = ql.front();
    tempTarget = qi->getTempTarget();
    size = qi->getSize();
    return qi->isChunkDownloaded(startPos, bytes);
}

} // namespace dcpp
