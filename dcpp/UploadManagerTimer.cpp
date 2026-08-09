/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadManager.h"

#include "ClientManager.h"
#include "ShareManager.h"
#include "Socket.h"
#include "Upload.h"
#include "UserConnection.h"
#include "Util.h"

namespace dcpp {

namespace {

const int64_t FIREBALL_BPS = 1024 * 1024;
const uint64_t FIREBALL_MS = 60 * 1000;
const time_t SERVER_UPTIME_S = 10 * 24 * 60 * 60;
const uint64_t SERVER_UPLOADED = 100ULL * 1024 * 1024 * 1024;
const int64_t SERVER_SHARE = (int64_t)(1.5 * 1024 * 1024 * 1024 * 1024);

} // namespace

void UploadManager::refreshShareStatus(uint64_t aTick) {
    if(!fireball) {
        if(getRunningAverage() >= FIREBALL_BPS) {
            if(fireballStartTick == 0)
                fireballStartTick = aTick;
            else if(aTick - fireballStartTick > FIREBALL_MS) {
                fireball = true;
                ClientManager::getInstance()->infoUpdated();
            }
        } else {
            fireballStartTick = 0;
        }
    }
    if(fireball || fileServer)
        return;
    if(Util::getUpTime() > SERVER_UPTIME_S &&
       Socket::getTotalUp() > SERVER_UPLOADED &&
       ShareManager::getInstance()->getShareSize() > SERVER_SHARE) {
        fileServer = true;
        ClientManager::getInstance()->infoUpdated();
    }
}

void UploadManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    {
        Lock l(cs);
        UploadList ticks;

        for (auto u : uploads) {
            if (u->getPos() > 0) {
                ticks.push_back(u);
                u->tick();
                u->getUserConnection().updateTransferSpeed(u->getStartPos() + u->getPos());
            }
        }

        // Only progressing uploads need UI refresh; idle slots stay until Starting/Complete/Failed.
        if (!ticks.empty())
            fire(UploadManagerListener::Tick(), ticks);
    }

    refreshShareStatus(aTick);
}

} // namespace dcpp
