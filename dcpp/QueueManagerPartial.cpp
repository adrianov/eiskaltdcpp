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
#include "ConnectionManager.h"
#include "Download.h"
#include "HashManager.h"
#include "PeerConnectHub.h"
#include "Segment.h"
#include "SettingsManager.h"

namespace dcpp {

bool QueueManager::handlePartialResult(const UserPtr& aUser, const string& hubHint, const TTHValue& tth, const QueueItem::PartialSource& partialSource, PartsInfo& outPartialInfo) {
    bool wantConnection = false;
    dcassert(outPartialInfo.empty());

    {
        Lock l(cs);

        // Locate target QueueItem in download queue
        auto ql = fileQueue.find(tth);

        if(ql.empty()){
            dcdebug("Not found in download queue\n");
            return false;
        }

        QueueItemPtr qi = ql[0];
        // don't add sources to finished files
        // this could happen when "Keep finished files in queue" is enabled
        if(qi->isFinished())
            return false;

        // Check min size
        if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
            dcassert(0);
            return false;
        }

        // Get my parts info
        int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
        if(blockSize == 0)
            blockSize = qi->getSize();
        qi->getPartialInfo(outPartialInfo, blockSize);

        // Any parts for me?
        wantConnection = qi->isNeededPart(partialSource.getPartialInfo(), blockSize);

        // If this user isn't a source and has no parts needed, ignore it
        QueueItem::SourceIter si = qi->getSource(aUser);
        if(si == qi->getSources().end()){
            si = qi->getBadSource(aUser);

            if(si != qi->getBadSources().end() && (si->isSet(QueueItem::Source::FLAG_BAD_TREE) || si->isSet(QueueItem::Source::FLAG_TTH_INCONSISTENCY)))
                return false;

            if(!wantConnection){
                if(si == qi->getBadSources().end())
                    return false;
            }else if(PeerConnectHub::isUnreachablePeer(aUser)){
                return false;
            }else{
                // add this user as partial file sharing source
                qi->addSource(HintedUser(aUser,""));
                si = qi->getSource(aUser);
                si->setFlag(QueueItem::Source::FLAG_PARTIAL);

                QueueItem::PartialSource* ps = new QueueItem::PartialSource(partialSource.getMyNick(),
                                                                            partialSource.getHubIpPort(), partialSource.getIp(), partialSource.getUdpPort());
                si->setPartialSource(ps);

                userQueue.add(qi, aUser);
                dcassert(si != qi->getSources().end());
                fire(QueueManagerListener::SourceAdded(), qi, HintedUser(aUser, hubHint));
                fire(QueueManagerListener::SourcesUpdated(), qi);
            }
        }

        // Update source's parts info
        if(si->getPartialSource()) {
            si->getPartialSource()->setPartialInfo(partialSource.getPartialInfo());
        }
    }

    // Connect to this user
    if(wantConnection)
        ConnectionManager::getInstance()->getDownloadConnection(HintedUser(aUser, hubHint));

    return true;
}

bool QueueManager::handlePartialSearch(const TTHValue& tth, PartsInfo& _outPartsInfo) {
    {
        Lock l(cs);

        // Locate target QueueItem in download queue
        auto ql = fileQueue.find(tth);

        if(ql.empty()){
            return false;
        }

        QueueItemPtr qi = ql[0];
        if(qi->getSize() < PARTIAL_SHARE_MIN_SIZE){
            return false;
        }

        int64_t blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
        if(blockSize == 0)
            blockSize = qi->getSize();
        qi->getPartialInfo(_outPartsInfo, blockSize);
    }

    return !_outPartsInfo.empty();
}

} // namespace dcpp
