/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "QueueManagerIndex.h"

#include "Download.h"
#include "HashManager.h"
#include "NaturalCompare.h"
#include "Segment.h"
#include "Transfer.h"

namespace dcpp {

void UserQueue::promotePartial(QueueItem* qi) {
    if(qi->getDownloadedBytes() == 0)
        return;

    for(auto& s: qi->getSources()) {
        auto& ulm = userQueue[qi->getPriority()];
        auto j = ulm.find(s.getUser());
        if(j == ulm.end())
            continue;

        auto& l = j->second;
        auto it = find(l.begin(), l.end(), qi);
        if(it == l.end() || it == l.begin())
            continue;

        l.erase(it);
        l.push_front(qi);
    }
}

QueueItem* UserQueue::getNext(const UserPtr& aUser, QueueItem::Priority minPrio, int64_t wantedSize,int64_t lastSpeed,bool allowRemove) {
    int p = QueueItem::LAST - 1;

    while(p >= minPrio) {
        auto i = userQueue[p].find(aUser);
        if(i == userQueue[p].end()) {
            --p;
            continue;
        }

        dcassert(!i->second.empty());
        bool retry = false;
        QueueItem* next = nullptr;

        // Prefer incomplete files that already have progress over not-started ones.
        // Within a pass, natural-sort full target path (dir + name) among alternatives.
        for(int pass = 0; pass < 2 && !next && !retry; ++pass) {
            for(auto qi: i->second) {
                const bool hasProgress = qi->getDownloadedBytes() > 0;
                if(pass == 0 ? !hasProgress : hasProgress)
                    continue;

                QueueItem::SourceConstIter source = qi->getSource(aUser);
                if(source->isSet(QueueItem::Source::FLAG_PARTIAL)) {
                    auto blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
                    if(blockSize == 0)
                        blockSize = qi->getSize();

                    Segment segment = qi->getNextSegment(blockSize, wantedSize, lastSpeed, source->getPartialSource());
                    if(allowRemove && segment.getStart() != -1 && segment.getSize() == 0) {
                        remove(qi, aUser);
                        qi->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
                        retry = true;
                        break;
                    }
                }
                if(!qi->isWaiting()) {
                    if(qi->getDownloads()[0]->getType() == Transfer::TYPE_TREE)
                        continue;
                    if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
                        auto blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
                        if(blockSize == 0)
                            blockSize = qi->getSize();
                        if(qi->getNextSegment(blockSize, wantedSize, lastSpeed, source->getPartialSource()).getSize() == 0) {
                            dcdebug("No segment for %s in %s, block " I64_FMT "\n",
                                    aUser->getCID().toBase32().c_str(), qi->getTarget().c_str(),
                                    static_cast<long long int>(blockSize));
                            continue;
                        }
                    }
                }
                if(!next || compareNatural(qi->getTarget(), next->getTarget()) < 0)
                    next = qi;
            }
        }

        if(retry)
            continue;
        if(next)
            return next;
        --p;
    }

    return NULL;
}

} // namespace dcpp
