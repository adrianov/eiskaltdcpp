/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "Download.h"
#include "LogManager.h"
#include "MerkleTree.h"
#include "format.h"

namespace dcpp {

namespace {

FastCriticalSection diskFullLogCs;
string diskFullLogVol;
uint64_t diskFullLogTick = 0;

class TreeOutputStream : public OutputStream {
public:
    TreeOutputStream(TigerTree& aTree) : tree(aTree), bufPos(0) {
        memset(&buf, 0, sizeof(buf));
    }

    virtual size_t write(const void* xbuf, size_t len) {
        size_t pos = 0;
        uint8_t* b = (uint8_t*)xbuf;
        while(pos < len) {
            size_t left = len - pos;
            if(bufPos == 0 && left >= TigerTree::BYTES) {
                tree.getLeaves().push_back(TTHValue(b + pos));
                pos += TigerTree::BYTES;
            } else {
                size_t bytes = min(TigerTree::BYTES - bufPos, left);
                memcpy(buf + bufPos, b + pos, bytes);
                bufPos += bytes;
                pos += bytes;
                if(bufPos == TigerTree::BYTES) {
                    tree.getLeaves().push_back(TTHValue(buf));
                    bufPos = 0;
                }
            }
        }
        return len;
    }

    virtual size_t flush() {
        return 0;
    }

private:
    TigerTree& tree;
    uint8_t buf[TigerTree::BYTES];
    size_t bufPos;
};

} // namespace

void QueueManager::setFile(Download* d) {
    if(d->getType() == Transfer::TYPE_FILE) {
        Lock l(cs);

        QueueItem* qi = fileQueue.find(d->getPath());
        if(!qi) {
            throw QueueException(_("Target removed"));
        }

        const string target = d->getDownloadTarget();

        if(d->getSegment().getStart() > 0) {
            if(File::getSize(target) != qi->getSize()) {
                throw QueueException(_("Target file is missing or wrong size"));
            }
        } else {
            File::ensureDirectory(target);
        }

        File* f = new File(target, File::WRITE, File::OPEN | File::CREATE | File::SHARED);

        if(f->getSize() != qi->getSize()) {
            f->setSize(qi->getSize());
        }

        f->setPos(d->getSegment().getStart());
        d->setFile(f);
    } else if(d->getType() == Transfer::TYPE_FULL_LIST) {
        {
            Lock l(cs);
            if(QueueItem* qi = fileQueue.find(d->getPath())) {
                if(qi->getSize() <= 0 && d->getSize() > 0)
                    qi->setSize(d->getSize());
            }
        }
        string target = d->getPath();
        File::ensureDirectory(target);

        if(d->isSet(Download::FLAG_XML_BZ_LIST)) {
            target += ".xml.bz2";
        } else {
            target += ".xml";
        }
        d->setFile(new File(target, File::WRITE, File::OPEN | File::TRUNCATE | File::CREATE));
    } else if(d->getType() == Transfer::TYPE_PARTIAL_LIST) {
        d->setFile(new StringRefOutputStream(d->getPFS()));
    } else if(d->getType() == Transfer::TYPE_TREE) {
        d->setFile(new TreeOutputStream(d->getTigerTree()));
    }
}

void QueueManager::moveFile(const string& source, const string& target) {
    File::ensureDirectory(target);
    if(File::getSize(source) > MOVER_LIMIT) {
        mover.moveFile(source, target);
    } else {
        moveFile_(source, target);
    }
}

void QueueManager::moveFile_(const string& source, const string& target) {
    try {
        File::renameFile(source, target);
        getInstance()->fire(QueueManagerListener::FileMoved(), target);
    } catch(const FileException& e1) {
        string newTarget = Util::getFilePath(source) + Util::getFileName(target);
        try {
            File::renameFile(source, newTarget);
            LogManager::getInstance()->message(str(F_("Unable to move %1% to %2% (%3%); renamed to %4%") %
                                                   Util::addBrackets(source) % Util::addBrackets(target) % e1.getError() % Util::addBrackets(newTarget)));
        } catch(const FileException& e2) {
            LogManager::getInstance()->message(str(F_("Unable to move %1% to %2% (%3%) nor to rename to %4% (%5%)") %
                                                   Util::addBrackets(source) % Util::addBrackets(target) % e1.getError() % Util::addBrackets(newTarget) % e2.getError()));
        }
    }
}

void QueueManager::moveStuckFile(QueueItem* qi) {
    moveFile(qi->getTempTarget(), qi->getTarget());

    if(qi->isFinished()) {
        userQueue.remove(qi);
    }

    string target = qi->getTarget();

    if(!BOOLSETTING(KEEP_FINISHED_FILES)) {
        fire(QueueManagerListener::Removed(), qi);
        fileQueue.remove(qi);
    } else {
        qi->addSegment(Segment(0, qi->getSize()));
        fire(QueueManagerListener::StatusUpdated(), qi);
    }

    fire(QueueManagerListener::RecheckAlreadyFinished(), target);
}

void QueueManager::handleDiskFull(const string& target) noexcept {
    const string volPath = Util::getFilePath(target);
    uint64_t free = 0;
    Util::getFreeBytes(volPath, free);

    bool log = false;
    {
        FastLock l(diskFullLogCs);
        const uint64_t now = GET_TICK();
        if(volPath != diskFullLogVol || now - diskFullLogTick > 60000) {
            diskFullLogTick = now;
            diskFullLogVol = volPath;
            log = true;
        }
    }
    if(log) {
        LogManager::getInstance()->message(str(F_("Not enough disk space in %1% (%2% free); paused %3%.") %
                                               Util::addBrackets(volPath) % Util::formatBytes(static_cast<int64_t>(free)) % Util::addBrackets(Util::getFileName(target))));
    }

    setPriority(target, QueueItem::PAUSED);
}

} // namespace dcpp
