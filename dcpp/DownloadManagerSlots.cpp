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
#include "DownloadManager.h"

#include "ClientManager.h"
#include "Download.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "UserConnection.h"
#include "format.h"

namespace dcpp {

namespace {

unordered_map<CID, uint64_t> lastSlotLog;

bool shouldLogSlotMsg(const UserPtr& user) {
    const uint64_t tick = GET_TICK();
    auto& last = lastSlotLog[user->getCID()];
    if(tick < last + 10 * 60 * 1000)
        return false;
    last = tick;
    return true;
}

} // namespace

void DownloadManager::checkIdle(const UserPtr& user) {
    Lock l(cs);
    for(auto uc: idlers) {
        if(uc->getUser() == user) {
            uc->updated();
            return;
        }
    }
}

bool DownloadManager::isWaitingUploadSlot(const UserPtr& user) {
    Lock l(cs);
    for(auto uc: idlers) {
        if(uc->getUser() == user)
            return true;
    }
    for(auto d: downloads) {
        if(d->getUser() != user)
            continue;
        const UserConnection::States s = d->getUserConnection().getState();
        if(s == UserConnection::STATE_SND || s == UserConnection::STATE_IDLE)
            return true;
    }
    return false;
}

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource, size_t queuePos) noexcept {
    noSlots(aSource, queuePos);
}

void DownloadManager::noSlots(UserConnection* aSource, size_t queuePos) {
    if(aSource->getState() != UserConnection::STATE_SND) {
        dcdebug("DM::noSlots Bad state, disconnecting\n");
        aSource->disconnect();
        return;
    }

    Download* d = aSource->getDownload();
    if(!d) {
        aSource->disconnect();
        return;
    }

    aSource->setLastActivity(GET_TICK());

    const string nick = ClientManager::getInstance()->getNickOrCid(aSource->getHintedUser());
    const string file = Util::addBrackets(d->getTargetFileName());
    if(shouldLogSlotMsg(aSource->getUser())) {
        string msg = queuePos > 0 ?
                str(F_("%1%: no upload slot, queue position %2% for %3%") % nick % queuePos % file) :
                str(F_("%1%: no upload slot, waiting in queue for %2%") % nick % file);
        LogManager::getInstance()->message(msg);
        dcdebug("DM::noSlots %s\n", msg.c_str());
    }

    fire(DownloadManagerListener::Queued(), d, queuePos);
}

void DownloadManager::on(UserConnectionListener::Send, UserConnection* aSource) noexcept {
    if(aSource->getState() != UserConnection::STATE_SND || !aSource->getDownload())
        return;

    Download* d = aSource->getDownload();
    startData(aSource, d->getStartPos(), d->getSize(),
              aSource->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET) && BOOLSETTING(COMPRESS_TRANSFERS));
}

} // namespace dcpp
