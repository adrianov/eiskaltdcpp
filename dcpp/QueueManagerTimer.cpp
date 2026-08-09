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
#include "HashManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "Socket.h"
#include "Util.h"
#include "queue/LocalMatch.h"

#ifdef WITH_DHT
#include "dht/IndexManager.h"
#endif

namespace dcpp {

namespace {

struct PartsInfoReqParam {
    PartsInfo parts;
    string tth;
    string myNick;
    string hubIpPort;
    string ip;
    string udpPort;
};

bool wasSearched(const StringList& recent, const string& target) {
    return find(recent.begin(), recent.end(), target) != recent.end();
}

QueueItem* findSearchCandidate(QueueItem* cand, QueueItem::StringIter start, QueueItem::StringIter end,
                               const StringList& recent) {
    for(auto i = start; i != end; ++i) {
        QueueItem* q = i->second;
        if(cand && q->isRunning())
            continue;
        if(q->isFinished() || q->isSet(QueueItem::FLAG_USER_LIST))
            continue;
        if(q->getPriority() == QueueItem::PAUSED || wasSearched(recent, q->getTarget()))
            continue;
        cand = q;
        if(cand->isWaiting())
            break;
    }
    return cand;
}

QueueItem* nextAutoSearch(QueueItem::StringMap& queue, StringList& recent) {
    while(recent.size() >= queue.size() || recent.size() > 30)
        recent.erase(recent.begin());
    if(queue.empty())
        return nullptr;

    auto i = queue.begin();
    advance(i, (QueueItem::StringMap::size_type)Util::rand((uint32_t)queue.size()));

    QueueItem* cand = findSearchCandidate(nullptr, i, queue.end(), recent);
    if(!cand || cand->isRunning())
        cand = findSearchCandidate(cand, queue.begin(), i, recent);
    if(cand)
        recent.push_back(cand->getTarget());
    return cand;
}

void collectPfsRequests(FileQueue& fileQueue, uint64_t aTick, vector<PartsInfoReqParam*>& params) {
    PFSSourceList sl;
    fileQueue.findPFSSources(sl);
    for(auto& i: sl) {
        QueueItem::PartialSource::Ptr source = i.first->getPartialSource();
        const QueueItem* qi = i.second;

        auto* param = new PartsInfoReqParam;
        int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
        if(blockSize == 0)
            blockSize = qi->getSize();
        qi->getPartialInfo(param->parts, blockSize);
        param->tth = qi->getTTH().toBase32();
        param->ip = source->getIp();
        param->udpPort = source->getUdpPort();
        param->myNick = source->getMyNick();
        param->hubIpPort = source->getHubIpPort();
        params.push_back(param);

        source->setPendingQueryCount(source->getPendingQueryCount() + 1);
        source->setNextQueryTime(aTick + 300000);
    }
}

void sendPfsRequests(const vector<PartsInfoReqParam*>& params) {
    for(auto* param: params) {
        try {
            AdcCommand cmd = SearchManager::getInstance()->toPSR(true, param->myNick, param->hubIpPort,
                    param->tth, param->parts);
            Socket s;
            s.writeTo(param->ip, param->udpPort, cmd.toString(ClientManager::getInstance()->getMyCID()));
        } catch(...) {
            dcdebug("Partial search caught error\n");
        }
        delete param;
    }
}

string pickAutoSearch(FileQueue& fileQueue, StringList& recent, uint64_t aTick, uint64_t& nextSearch) {
    if(!BOOLSETTING(AUTO_SEARCH) || aTick < nextSearch || fileQueue.getSize() == 0)
        return Util::emptyString;

    QueueItem* qi = nextAutoSearch(fileQueue.getQueue(), recent);
    if(!qi)
        return Util::emptyString;

    nextSearch = aTick + (SETTING(AUTO_SEARCH_TIME) * 60000);
    if(BOOLSETTING(REPORT_ALTERNATES)) {
        string name = qi->getTargetFileName();
        if(name.empty())
            name = qi->getTarget();
        if(!name.empty())
            LogManager::getInstance()->message(str(F_("Auto-searching for more sources: %1%") % name));
    }
    return qi->getTTH().toBase32();
}

} // namespace

void QueueManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
    if(ConnectionManager::getInstance()->isShuttingDown())
        return;

    localMatch->sweep();

    string searchString;
    vector<PartsInfoReqParam*> params;
    TTHValue* tthPub = nullptr;
    {
        Lock l(cs);
        collectPfsRequests(fileQueue, aTick, params);
#ifdef WITH_DHT
        if(BOOLSETTING(USE_DHT) && SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_PASSIVE)
            tthPub = fileQueue.findPFSPubTTH();
#endif
        searchString = pickAutoSearch(fileQueue, autoSearchRecent, aTick, nextSearch);
    }

    sendPfsRequests(params);

    if(tthPub) {
#ifdef WITH_DHT
        dht::IndexManager::getInstance()->publishPartialFile(*tthPub);
#endif
        delete tthPub;
    }

    if(!searchString.empty())
        SearchManager::getInstance()->search(searchString, 0, SearchManager::TYPE_TTH,
                SearchManager::SIZE_DONTCARE, "auto");
}

} // namespace dcpp
