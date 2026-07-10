/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "ConnectionManager.h"

#include "ClientManager.h"
#include "CryptoManager.h"
#include "PeerConnectLog.h"
#include "UserConnection.h"

namespace dcpp {

void ConnectionManager::on(UserConnectionListener::MyNick, UserConnection* aSource, const string& aNick) noexcept {
    if(aSource->getState() != UserConnection::STATE_SUPNICK) {
        dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
        return;
    }

    dcassert(!aNick.empty());
    dcdebug("ConnectionManager::onMyNick %p, %s\n", (void*)aSource, aNick.c_str());

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        pair<string, string> i = expectedConnections.remove(aNick);
        if(i.second.empty()) {
            dcassert(i.first.empty());
            dcdebug("Unknown incoming connection from %s\n", aNick.c_str());
            PeerConnectLog::incomingReject(aNick, _("unexpected nick (not awaiting this user)"));
            putConnection(aSource);
            return;
        }
        aSource->setToken(i.first);
        aSource->setHubUrl(i.second);
        aSource->setEncoding(ClientManager::getInstance()->findHubEncoding(i.second));
    }

    // Hub user list / CID use UTF-8 (NmdcHubLine toUtf8); $MyNick on the wire uses hub encoding.
    const string nick = Text::toUtf8(aNick, aSource->getEncoding());
    const CID wireCid = ClientManager::getInstance()->makeCid(nick, aSource->getHubUrl());
    const string& hubUrl = aSource->getHubUrl();

    ConnectionQueueItem::Ptr matchedCqi;
    ConnectionQueueItem::List pending;
    {
        Lock l(cs);
        for(auto& cqi: downloads) {
            if(cqi->getState() == ConnectionQueueItem::CONNECTING ||
                    cqi->getState() == ConnectionQueueItem::WAITING)
                pending.push_back(cqi);
        }
    }

    for(auto& cqi: pending) {
        // CID is hub-scoped for NMDC; never attach another hub's download queue item.
        if(cqi->getUser().user->getCID() == wireCid) {
            matchedCqi = cqi;
            break;
        }
    }

    if(!matchedCqi) {
        for(auto& cqi: pending) {
            if(!cqi->getUser().hint.empty() && !hubUrl.empty() &&
                    cqi->getUser().hint != hubUrl)
                continue;
            for(const auto& hubNick : ClientManager::getInstance()->getNicks(
                    cqi->getUser().user->getCID(), hubUrl)) {
                if(nickWireMatch(nick, hubNick)) {
                    matchedCqi = cqi;
                    break;
                }
            }
            if(matchedCqi)
                break;
        }
    }

    if(matchedCqi) {
        Lock l(cs);
        aSource->setUser(matchedCqi->getUser());
        aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
    }

    if(!aSource->getUser()) {
        UserPtr user = ClientManager::getInstance()->findNmdcUser(nick, aSource->getHubUrl());
        if(!user && aSource->isSet(UserConnection::FLAG_INCOMING))
            user = ClientManager::getInstance()->getUser(nick, aSource->getHubUrl());
        if(!user) {
            dcdebug("CM::onMyNick Outgoing connection from unknown user %s\n", nick.c_str());
            PeerConnectLog::incomingReject(nick, _("user not online on hub"));
            putConnection(aSource);
            return;
        }
        if(!aSource->isSet(UserConnection::FLAG_INCOMING) &&
                !ClientManager::getInstance()->isOnline(user)) {
            dcdebug("CM::onMyNick Outgoing connection from offline user %s\n", nick.c_str());
            PeerConnectLog::incomingReject(nick, _("user not online on hub"));
            putConnection(aSource);
            return;
        }
        aSource->setUser(user);
        aSource->setFlag(UserConnection::FLAG_UPLOAD);
    }

    if(ClientManager::getInstance()->isOp(aSource->getUser(), aSource->getHubUrl()))
        aSource->setFlag(UserConnection::FLAG_OP);

    ClientManager::getInstance()->setIPUser(aSource->getUser(), aSource->getRemoteIp());

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        aSource->myNick(aSource->getToken());
        aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk());
    }

    aSource->setState(UserConnection::STATE_LOCK);
}

} // namespace dcpp
