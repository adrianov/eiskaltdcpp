/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "DownloadManager.h"

#include "Download.h"
#include "QueueManager.h"
#include "SettingsManager.h"
#include "Transfer.h"
#include "UserConnection.h"

namespace dcpp {

void DownloadManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    typedef vector<pair<string, UserPtr> > TargetList;
    TargetList dropTargets;

    {
        Lock l(cs);

        DownloadList tickList;
        for(auto i: downloads) {
            if(i->getPos() > 0) {
                tickList.push_back(i);
                i->tick();
            }
        }

        if(!tickList.empty())
            fire(DownloadManagerListener::Tick(), tickList);

        if((uint32_t)(aTick / 1000) % SETTING(AUTODROP_INTERVAL) == 0) {
            for(auto d: downloads) {
                uint64_t timeElapsed = aTick - d->getStart();
                uint64_t timeInactive = aTick - d->getUserConnection().getLastActivity();
                uint64_t bytesDownloaded = d->getPos();
                bool timeElapsedOk = timeElapsed >= (uint32_t)SETTING(AUTODROP_ELAPSED) * 1000;
                bool timeInactiveOk = timeInactive <= (uint32_t)SETTING(AUTODROP_INACTIVITY) * 1000;
                bool speedTooLow = timeElapsedOk && timeInactiveOk && bytesDownloaded > 0 ?
                            bytesDownloaded / timeElapsed * 1000 < (uint32_t)SETTING(AUTODROP_SPEED) : false;
                bool isUserList = d->getType() == Transfer::TYPE_FULL_LIST;
                bool onlineSourcesOk = isUserList ?
                            true : QueueManager::getInstance()->countOnlineSources(d->getPath()) >= SETTING(AUTODROP_MINSOURCES);
                bool filesizeOk = !isUserList && d->getSize() >= ((int64_t)SETTING(AUTODROP_FILESIZE)) * 1024;
                bool dropIt = (isUserList && BOOLSETTING(AUTODROP_FILELISTS)) ||
                        (filesizeOk && BOOLSETTING(AUTODROP_ALL));
                if(speedTooLow && onlineSourcesOk && dropIt) {
                    if(BOOLSETTING(AUTODROP_DISCONNECT) && isUserList) {
                        d->getUserConnection().disconnect();
                    } else {
                        dropTargets.emplace_back(d->getPath(), d->getUser());
                    }
                }
            }
        }
    }
    for(auto& i: dropTargets) {
        QueueManager::getInstance()->removeSource(i.first, i.second, QueueItem::Source::FLAG_SLOW_SOURCE);
    }
}

} // namespace dcpp
