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

#include "HashManager.h"
#include "Segment.h"
#include "SettingsManager.h"
#include "Util.h"

#ifdef WITH_DHT
#include "dht/Constants.h"
#endif

namespace dcpp {

QueueItem* FileQueue::add(const string& aTarget, int64_t aSize,
                          int aFlags, QueueItem::Priority p, const string& aTempTarget,
                          time_t aAdded, const TTHValue& root)
{
    if(p == QueueItem::DEFAULT) {
        p = QueueItem::NORMAL;
        if(aSize <= SETTING(PRIO_HIGHEST_SIZE)*1024) {
            p = QueueItem::HIGHEST;
        } else if(aSize <= SETTING(PRIO_HIGH_SIZE)*1024) {
            p = QueueItem::HIGH;
        } else if(aSize <= SETTING(PRIO_NORMAL_SIZE)*1024) {
            p = QueueItem::NORMAL;
        } else if(aSize <= SETTING(PRIO_LOW_SIZE)*1024) {
            p = QueueItem::LOW;
        } else if(SETTING(PRIO_LOWEST)) {
            p = QueueItem::LOWEST;
        }
    }

    QueueItem* qi = new QueueItem(aTarget, aSize, p, aFlags, aAdded, root);

    if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
        qi->setPriority(QueueItem::HIGHEST);
    }

    qi->setTempTarget(aTempTarget);

    add(qi);
    return qi;
}

void FileQueue::add(QueueItem* qi) {
    if(lastInsert == queue.end())
        lastInsert = queue.emplace(const_cast<string*>(&qi->getTarget()), qi).first;
    else
        lastInsert = queue.insert(lastInsert, make_pair(const_cast<string*>(&qi->getTarget()), qi));
}

void FileQueue::remove(QueueItem* qi) {
    if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
        ++lastInsert;
    queue.erase(const_cast<string*>(&qi->getTarget()));
    delete qi;
}

QueueItem* FileQueue::find(const string& target) {
    auto i = queue.find(const_cast<string*>(&target));
    return (i == queue.end()) ? NULL : i->second;
}

QueueItemList FileQueue::find(const TTHValue& tth) {
    QueueItemList ql;
    for(auto& i: queue) {
        auto qi = i.second;
        if(qi->getTTH() == tth) {
            ql.push_back(qi);
        }
    }
    return ql;
}

void FileQueue::move(QueueItem* qi, const string& aTarget) {
    if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
        lastInsert = queue.end();
    queue.erase(const_cast<string*>(&qi->getTarget()));
    qi->setTarget(aTarget);
    add(qi);
}

void FileQueue::findPFSSources(PFSSourceList& sl)
{
    typedef multimap<time_t, pair<QueueItem::SourceConstIter, const QueueItem*> > Buffer;
    Buffer buffer;
    uint64_t now = GET_TICK();

    for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i) {
        const QueueItem* q = i->second;

        if(q->getSize() < PARTIAL_SHARE_MIN_SIZE) continue;

        const QueueItem::SourceList& sources = q->getSources();
        const QueueItem::SourceList& badSources = q->getBadSources();

        for(QueueItem::SourceConstIter j = sources.begin(); j != sources.end(); ++j) {
            if( (*j).isSet(QueueItem::Source::FLAG_PARTIAL) && (*j).getPartialSource()->getNextQueryTime() <= now &&
                    (*j).getPartialSource()->getPendingQueryCount() < 10 && Util::toInt((*j).getPartialSource()->getUdpPort()) > 0)
            {
                buffer.emplace((*j).getPartialSource()->getNextQueryTime(), make_pair(j, q));
            }
        }

        for(QueueItem::SourceConstIter j = badSources.begin(); j != badSources.end(); ++j) {
            if( (*j).isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY) == false && (*j).isSet(QueueItem::Source::FLAG_PARTIAL) &&
                    (*j).getPartialSource()->getNextQueryTime() <= now && (*j).getPartialSource()->getPendingQueryCount() < 10 &&
                    Util::toInt((*j).getPartialSource()->getUdpPort()) > 0)
            {
                buffer.emplace((*j).getPartialSource()->getNextQueryTime(), make_pair(j, q));
            }
        }
    }

    dcassert(sl.empty());
    const uint8_t maxElements = 10;
    sl.reserve(maxElements);
    for(Buffer::iterator i = buffer.begin(); i != buffer.end() && sl.size() < maxElements; ++i){
        sl.push_back(i->second);
    }
}

#ifdef WITH_DHT
TTHValue* FileQueue::findPFSPubTTH()
{
    uint64_t now = GET_TICK();
    QueueItem::Ptr cand = NULL;

    for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i)
    {
        QueueItem::Ptr qi = i->second;
        if(qi && qi->getSize() >= PARTIAL_SHARE_MIN_SIZE && now >= qi->getNextPublishingTime() && qi->getPriority() > QueueItem::PAUSED && qi->isRunning())
        {
            if(cand == NULL || cand->getNextPublishingTime() > qi->getNextPublishingTime() || (cand->getNextPublishingTime() == qi->getNextPublishingTime() && cand->getPriority() < qi->getPriority()) )
            {
                if(qi->getDownloadedBytes() > (int64_t)HashManager::getInstance()->getBlockSize(qi->getTTH()))
                    cand = qi;
            }
        }
    }
    if(cand)
    {
        cand->setNextPublishingTime(now + PFS_REPUBLISH_TIME);
        return new TTHValue(cand->getTTH());
    }
    return NULL;
}
#endif

} // namespace dcpp
