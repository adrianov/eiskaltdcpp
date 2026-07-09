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
#include "Segment.h"
#include "Transfer.h"
#include "Util.h"

namespace dcpp {

void UserQueue::add(QueueItem* qi) {
    for(auto& i: qi->getSources()) {
        add(qi, i.getUser());
    }
}

void UserQueue::add(QueueItem* qi, const UserPtr& aUser) {
    auto& l = userQueue[qi->getPriority()][aUser];

    if(qi->getDownloadedBytes() > 0) {
        l.push_front(qi);
    } else {
        l.push_back(qi);
    }
}

QueueItem* UserQueue::getNext(const UserPtr& aUser, QueueItem::Priority minPrio, int64_t wantedSize,int64_t lastSpeed,bool allowRemove) {
    int p = QueueItem::LAST - 1;
    string lastError = Util::emptyString;

    do {
        auto i = userQueue[p].find(aUser);
        if(i != userQueue[p].end()) {
            dcassert(!i->second.empty());
            for(auto qi: i->second) {
                QueueItem::SourceConstIter source = qi->getSource(aUser);
                if(source->isSet(QueueItem::Source::FLAG_PARTIAL)) {
                    auto blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
                    if(blockSize == 0)
                        blockSize = qi->getSize();

                    Segment segment = qi->getNextSegment(blockSize, wantedSize, lastSpeed, source->getPartialSource());
                    if(allowRemove && segment.getStart() != -1 && segment.getSize() == 0) {
                        remove(qi, aUser);
                        qi->removeSource(aUser, QueueItem::Source::FLAG_NO_NEED_PARTS);
                        lastError = _("No needed part");
                        p++;
                        break;
                    }
                }
                if(qi->isWaiting()) {
                    return qi;
                }

                if(qi->getDownloads()[0]->getType() == Transfer::TYPE_TREE) {
                    continue;
                }
                if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
                    auto blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
                    if(blockSize == 0)
                        blockSize = qi->getSize();
                    if(qi->getNextSegment(blockSize, wantedSize,lastSpeed, source->getPartialSource()).getSize() == 0) {
                        dcdebug("No segment for %s in %s, block " I64_FMT "\n",
                                aUser->getCID().toBase32().c_str(), qi->getTarget().c_str(),
                                static_cast<long long int>(blockSize));
                        continue;
                    }
                }
                return qi;
            }
        }
        p--;
    } while(p >= minPrio);

    return NULL;
}

void UserQueue::addDownload(QueueItem* qi, Download* d) {
    qi->getDownloads().push_back(d);
    running[d->getUser()] = qi;
}

void UserQueue::removeDownload(QueueItem* qi, const UserPtr& user) {
    running.erase(user);

    for(auto i = qi->getDownloads().begin(); i != qi->getDownloads().end(); ++i) {
        if((*i)->getUser() == user) {
            qi->getDownloads().erase(i);
            break;
        }
    }
}

void UserQueue::setPriority(QueueItem* qi, QueueItem::Priority p) {
    remove(qi, false);
    qi->setPriority(p);
    add(qi);
}

int64_t UserQueue::getQueued(const UserPtr& aUser) const {
    int64_t total = 0;
    for(size_t i = QueueItem::LOWEST; i < QueueItem::LAST; ++i) {
        const auto& ulm = userQueue[i];
        auto iulm = ulm.find(aUser);
        if(iulm == ulm.end()) {
            continue;
        }

        for(auto& qi: iulm->second) {
            if(qi->getSize() != -1) {
                total += qi->getSize() - qi->getDownloadedBytes();
            }
        }
    }
    return total;
}

QueueItem* UserQueue::getRunning(const UserPtr& aUser) {
    auto i = running.find(aUser);
    return (i == running.end()) ? 0 : i->second;
}

void UserQueue::remove(QueueItem* qi, bool removeRunning) {
    for(auto& i: qi->getSources()) {
        remove(qi, i.getUser(), removeRunning);
    }
}

void UserQueue::remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning) {
    if(removeRunning && qi == getRunning(aUser)) {
        removeDownload(qi, aUser);
    }

    dcassert(qi->isSource(aUser));
    auto& ulm = userQueue[qi->getPriority()];
    auto j = ulm.find(aUser);
    dcassert(j != ulm.end());
    auto& l = j->second;
    auto i = find(l.begin(), l.end(), qi);
    dcassert(i != l.end());
    l.erase(i);

    if(l.empty()) {
        ulm.erase(j);
    }
}

} // namespace dcpp
