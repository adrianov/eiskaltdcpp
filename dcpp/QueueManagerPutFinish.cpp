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
#include "Download.h"
#include "File.h"
#include "HashManager.h"
#include "ListCache.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "Transfer.h"
#include "format.h"

namespace dcpp {

void QueueManager::putDownloadBody(Download* d, bool finished, HintedUserList& getConn,
        string& fl_fname, HintedUser& fl_user, int& fl_flag) {
    if(d->getType() == Transfer::TYPE_PARTIAL_LIST) {
        QueueItem* q = nullptr;
        try {
            q = fileQueue.find(getListPath(d->getHintedUser()));
        } catch(const Exception&) { }
        if(!q)
            return;
        if(!d->getPFS().empty()) {
            if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(d->getUser()) != directories.end()) ||
                    (q->isSet(QueueItem::FLAG_MATCH_QUEUE)) )
            {
                dcassert(finished);
                fl_fname = d->getPFS();
                fl_user = d->getHintedUser();
                fl_flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? (QueueItem::FLAG_DIRECTORY_DOWNLOAD) : 0)
                        | (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0) | QueueItem::FLAG_TEXT;
            } else {
                fire(QueueManagerListener::PartialList(), d->getHintedUser(), d->getPFS());
            }
        } else {
            dcassert(!finished);
            fl_flag = q->getFlags() & ~QueueItem::FLAG_PARTIAL_LIST;
        }
        fire(QueueManagerListener::Removed(), q);
        userQueue.remove(q);
        fileQueue.remove(q);
        return;
    }

    QueueItem* q = fileQueue.find(d->getPath());
    if(!q) {
        if(d->getType() != Transfer::TYPE_TREE) {
            if(!d->getTempTarget().empty() && (d->getType() == Transfer::TYPE_FULL_LIST || d->getTempTarget() != d->getPath()))
                File::deleteFile(d->getTempTarget());
        }
        return;
    }

    if(d->getType() == Transfer::TYPE_FULL_LIST) {
        if(d->isSet(Download::FLAG_XML_BZ_LIST))
            q->setFlag(QueueItem::FLAG_XML_BZLIST);
        else
            q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
    }

    if(!finished) {
        if(d->getType() != Transfer::TYPE_TREE) {
            if(q->getDownloadedBytes() == 0)
                q->setTempTarget(Util::emptyString);
            if(q->isSet(QueueItem::FLAG_USER_LIST))
                File::deleteFile(q->getListName());
            if(d->getType() == Transfer::TYPE_FILE) {
                int64_t downloaded = d->getPos();
                downloaded -= downloaded % d->getTigerTree().getBlockSize();
                if(downloaded > 0) {
                    const bool wasEmpty = q->getDownloadedBytes() == 0;
                    q->addSegment(Segment(d->getStartPos(), downloaded));
                    if(wasEmpty)
                        userQueue.promotePartial(q);
                    setDirty();
                }
            }
        }
        if(q->getPriority() != QueueItem::PAUSED)
            q->getOnlineUsers(getConn);
        userQueue.removeDownload(q, d->getUser());
        fire(QueueManagerListener::StatusUpdated(), q);
        return;
    }

    if(d->getType() == Transfer::TYPE_TREE) {
        dcassert(d->getTreeValid());
        HashManager::getInstance()->addTree(d->getTigerTree());
        userQueue.removeDownload(q, d->getUser());
        fire(QueueManagerListener::StatusUpdated(), q);
        return;
    }

    string dir;
    bool crcError = false;
    const bool wasEmpty = q->getDownloadedBytes() == 0;
    if(d->getType() == Transfer::TYPE_FULL_LIST) {
        dir = q->getTempTarget();
        const string listName = q->getListName();
        const int64_t listBytes = File::getSize(listName);
        const int64_t share = ClientManager::getInstance()->getBytesShared(d->getUser());
        if(!ListCache::isPlausibleList(listBytes) ||
                !ListCache::listHasEntries(d->getHintedUser(), listName)) {
            // Empty or unparseable stub: drop file and queue item, or DownloadManager loops.
            try { File::deleteFile(listName); } catch(const Exception&) { }
            LogManager::getInstance()->message(str(F_("%1%: rejected implausible file list (%2% bytes, share %3%)")
                    % ClientManager::getInstance()->getNickOrCid(d->getHintedUser())
                    % listBytes % Util::formatBytes(share)));
            fire(QueueManagerListener::Removed(), q);
            userQueue.remove(q);
            fileQueue.remove(q);
            setDirty();
            return;
        }
        q->addSegment(Segment(0, q->getSize()));
        ListCache::saveListMeta(d->getUser()->getCID(), share, listBytes);
    } else if(d->getType() == Transfer::TYPE_FILE) {
        q->addSegment(d->getSegment());
        if(wasEmpty)
            userQueue.promotePartial(q);
    }

    if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(d->getUser()) != directories.end()) ||
            (q->isSet(QueueItem::FLAG_MATCH_QUEUE)) )
    {
        fl_fname = q->getListName();
        fl_user = d->getHintedUser();
        fl_flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
                | (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
    }

    if(q->isFinished() && BOOLSETTING(SFV_CHECK))
        crcError = checkSfv(q, d);

    if(d->getType() != Transfer::TYPE_FILE || q->isFinished()) {
        if(d->getType() == Transfer::TYPE_FILE && !d->getTempTarget().empty() &&
                (Util::stricmp(d->getPath().c_str(), d->getTempTarget().c_str()) != 0)) {
            moveFile(d->getTempTarget(), d->getPath());
        }
        if(BOOLSETTING(LOG_FINISHED_DOWNLOADS) && d->getType() == Transfer::TYPE_FILE)
            logFinishedDownload(q, d, crcError);

        fire(QueueManagerListener::Finished(), q, dir, d->getAverageSpeed());
        userQueue.remove(q);

        if(!BOOLSETTING(KEEP_FINISHED_FILES) || d->getType() == Transfer::TYPE_FULL_LIST) {
            fire(QueueManagerListener::Removed(), q);
            fileQueue.remove(q);
        } else {
            fire(QueueManagerListener::StatusUpdated(), q);
        }
    } else {
        userQueue.removeDownload(q, d->getUser());
        fire(QueueManagerListener::StatusUpdated(), q);
    }
    setDirty();
}

} // namespace dcpp
