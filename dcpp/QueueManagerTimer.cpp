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
#include "HashManager.h"
#include "LogManager.h"
#include "QueueAutoSearch.h"
#include "SearchManager.h"
#include "SettingsManager.h"

#ifdef WITH_DHT
#include "dht/IndexManager.h"
#endif

namespace dcpp {

struct PartsInfoReqParam{
    PartsInfo       parts;
    string          tth;
    string          myNick;
    string          hubIpPort;
    string          ip;
    string          udpPort;
};

void QueueManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
    string searchString;
    SearchManager::TypeModes searchType = SearchManager::TYPE_TTH;
    vector<const PartsInfoReqParam*> params;
    TTHValue* tthPub = NULL;
    {
        Lock l(cs);
        //find max 10 pfs sources to exchange parts
        //the source basis interval is 5 minutes
        PFSSourceList sl;
        fileQueue.findPFSSources(sl);

        for(PFSSourceList::const_iterator i = sl.begin(); i != sl.end(); ++i){
            QueueItem::PartialSource::Ptr source = (*i->first).getPartialSource();
            const QueueItem* qi = i->second;

            PartsInfoReqParam* param = new PartsInfoReqParam;

            int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
            if(blockSize == 0)
                blockSize = qi->getSize();
            qi->getPartialInfo(param->parts, blockSize);

            param->tth = qi->getTTH().toBase32();
            param->ip  = source->getIp();
            param->udpPort = source->getUdpPort();
            param->myNick = source->getMyNick();
            param->hubIpPort = source->getHubIpPort();

            params.push_back(param);

            source->setPendingQueryCount(source->getPendingQueryCount() + 1);
            source->setNextQueryTime(aTick + 300000);               // 5 minutes
        }

#ifdef WITH_DHT
        if(BOOLSETTING(USE_DHT) && SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_PASSIVE)
            tthPub = fileQueue.findPFSPubTTH();
#endif

        if(BOOLSETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
            const AutoSearchPick pick = QueueAutoSearch::pickAlternating(fileQueue.getQueue(),
                recent, recentNames, nextAutoSearchTTH);
            if(pick.item && !pick.query.empty()) {
                searchString = pick.query;
                searchType = pick.type;
                nextSearch = aTick + (SETTING(AUTO_SEARCH_TIME) * 60000);
                nextAutoSearchTTH = (pick.type != SearchManager::TYPE_TTH);
                if(BOOLSETTING(REPORT_ALTERNATES)) {
                    string name = pick.item->getTargetFileName();
                    if(name.empty())
                        name = pick.item->getTarget();
                    if(!name.empty()) {
                        if(pick.type == SearchManager::TYPE_TTH)
                            LogManager::getInstance()->message(str(F_("Searching TTH alternates for: %1%") % name));
                        else
                            LogManager::getInstance()->message(str(F_("Searching filename alternates for: %1%") % name));
                    }
                }
            }
        }
    }
    // Request parts info from partial file sharing sources
    for(vector<const PartsInfoReqParam*>::const_iterator i = params.begin(); i != params.end(); ++i){
        const PartsInfoReqParam* param = *i;

        try {
            AdcCommand cmd = SearchManager::getInstance()->toPSR(true, param->myNick, param->hubIpPort, param->tth, param->parts);
            Socket s;
            s.writeTo(param->ip, param->udpPort, cmd.toString(ClientManager::getInstance()->getMyCID()));
        } catch(...) {
            dcdebug("Partial search caught error\n");
        }

        delete param;
    }

    // DHT PFS announce
    if(tthPub)
    {
#ifdef WITH_DHT
        dht::IndexManager::getInstance()->publishPartialFile(*tthPub);
#endif
        delete tthPub;
    }

    if(!searchString.empty())
        SearchManager::getInstance()->search(searchString, 0, searchType, SearchManager::SIZE_DONTCARE, "auto");
}

// NOTE: freedcpp: begin

} // namespace dcpp
