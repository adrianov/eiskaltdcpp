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
#include "QueueManagerWorkers.h"

#include "Exception.h"
#include "File.h"
#include "HashManager.h"
#include "MerkleCheckOutputStream.h"
#include "QueueManager.h"
#include "Segment.h"

namespace dcpp {

void Rechecker::add(const string& file) {
    Lock l(cs);
    files.push_back(file);
    if(!active) {
        active = true;
        start();
    }
}

int Rechecker::run() {
    while(true) {
        string file;
        {
            Lock l(cs);
            auto i = files.begin();
            if(i == files.end()) {
                active = false;
                return 0;
            }
            file = *i;
            files.erase(i);
        }

        QueueItem* q;
        int64_t tempSize;
        TTHValue tth;

        {
            Lock l(qm->cs);

            q = qm->fileQueue.find(file);
            if(!q || q->isSet(QueueItem::FLAG_USER_LIST))
                continue;

            qm->fire(QueueManagerListener::RecheckStarted(), q->getTarget());
            dcdebug("Rechecking %s\n", file.c_str());

            tempSize = File::getSize(q->getTempTarget());

            if(tempSize == -1) {
                qm->fire(QueueManagerListener::RecheckNoFile(), q->getTarget());
                continue;
            }

            if(tempSize < 64*1024) {
                qm->fire(QueueManagerListener::RecheckFileTooSmall(), q->getTarget());
                continue;
            }

            if(tempSize != q->getSize()) {
                File(q->getTempTarget(), File::WRITE, File::OPEN).setSize(q->getSize());
            }

            if(q->isRunning()) {
                qm->fire(QueueManagerListener::RecheckDownloadsRunning(), q->getTarget());
                continue;
            }

            tth = q->getTTH();
        }

        TigerTree tt;
        bool gotTree = HashManager::getInstance()->getTree(tth, tt);

        string tempTarget;

        {
            Lock l(qm->cs);

            q = qm->fileQueue.find(file);
            if(!q)
                continue;

            if(!gotTree) {
                qm->fire(QueueManagerListener::RecheckNoTree(), q->getTarget());
                continue;
            }

            q->resetDownloaded();
            tempTarget = q->getTempTarget();
        }

        int64_t startPos=0;
        DummyOutputStream dummy;
        int64_t blockSize = tt.getBlockSize();
        bool hasBadBlocks = false;

        vector<uint8_t> buf((size_t)min((int64_t)1024*1024, blockSize));

        typedef pair<int64_t, int64_t> SizePair;
        typedef vector<SizePair> Sizes;
        Sizes sizes;

        {
            File inFile(tempTarget, File::READ, File::OPEN);

            while(startPos < tempSize) {
                try {
                    MerkleCheckOutputStream<TigerTree, false> check(tt, &dummy, startPos);

                    inFile.setPos(startPos);
                    int64_t bytesLeft = min((tempSize - startPos),blockSize);
                    int64_t segmentSize = bytesLeft;
                    while(bytesLeft > 0) {
                        size_t n = (size_t)min((int64_t)buf.size(), bytesLeft);
                        size_t nr = inFile.read(&buf[0], n);
                        check.write(&buf[0], nr);
                        bytesLeft -= nr;
                        if(bytesLeft > 0 && nr == 0) {
                            throw Exception();
                        }
                    }
                    check.flush();

                    sizes.emplace_back(startPos, segmentSize);
                } catch(const Exception&) {
                    hasBadBlocks = true;
                    dcdebug("Found bad block at " I64_FMT "\n", static_cast<long long int>(startPos));
                }
                startPos += blockSize;
            }
        }

        Lock l(qm->cs);

        q = qm->fileQueue.find(file);
        if(!q)
            continue;

        if(!hasBadBlocks) {
            qm->moveStuckFile(q);
            continue;
        }

        for(auto& i : sizes)
            q->addSegment(Segment(i.first, i.second));

        qm->rechecked(q);
    }
    return 0;
}

} // namespace dcpp
