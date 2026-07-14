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

namespace {

bool nickMatchesHub(const string& utf8Nick, const CID& cid, const string& hubUrl) {
    if(utf8Nick.empty() || hubUrl.empty())
        return false;
    if(ClientManager::getInstance()->makeCid(utf8Nick, hubUrl) == cid)
        return true;
    // Hub-private nicks only — never match another hub's identity.
    for(const auto& hubNick : ClientManager::getInstance()->getNicks(cid, hubUrl, true)) {
        if(nickWireMatch(utf8Nick, hubNick))
            return true;
    }
    return false;
}

} // namespace

void ConnectionManager::on(UserConnectionListener::MyNick, UserConnection* aSource, const string& aNick) noexcept {
    if(aSource->getState() != UserConnection::STATE_SUPNICK) {
        dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
        return;
    }

    if(aNick.empty()) {
        PeerConnectLog::incomingReject(Util::emptyString, _("empty $MyNick"));
        putConnection(aSource);
        return;
    }

    dcdebug("ConnectionManager::onMyNick %p, %s\n", (void*)aSource, aNick.c_str());

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        // Exact wire match (ASCII / legacy hub-encoded keys), then encoding-aware UTF-8.
        // nmdcExpect() keeps only the latest hub for this nick, so the first hit is correct.
        StringPair i = expectedConnections.removeExact(aNick);
        if(i.second.empty()) {
            i = expectedConnections.removeIf([&](const string& utf8Key, const string& hubUrl) {
                const string enc = ClientManager::getInstance()->findHubEncoding(hubUrl);
                const string wireUtf8 = Text::toUtf8(aNick, enc);
                if(nickWireMatch(wireUtf8, utf8Key) || wireUtf8 == utf8Key)
                    return true;
                // Peer sent hub-encoded bytes while key is UTF-8.
                return Text::fromUtf8(utf8Key, enc) == aNick;
            });
        }
        if(i.second.empty()) {
            dcdebug("Unknown incoming connection from %s\n", aNick.c_str());
            PeerConnectLog::incomingReject(aNick, _("unexpected nick (not awaiting this user)"));
            putConnection(aSource);
            return;
        }
        aSource->setToken(i.first);
        aSource->setHubUrl(i.second);
        aSource->setEncoding(ClientManager::getInstance()->findHubEncoding(i.second));
    }

    // Hub user list / CID use UTF-8; $MyNick on the wire uses hub encoding.
    const string nick = Text::toUtf8(aNick, aSource->getEncoding());
    if(nick.empty()) {
        PeerConnectLog::incomingReject(aNick, _("empty $MyNick after encoding"));
        putConnection(aSource);
        return;
    }

    const CID wireCid = ClientManager::getInstance()->makeCid(nick, aSource->getHubUrl());
    const string& hubUrl = aSource->getHubUrl();

    {
        // One lock for match + bind: CQIs may be deleted by the timer thread.
        Lock l(cs);
        ConnectionQueueItem::Ptr matchedCqi = findDownloadCqiForHub(hubUrl, wireCid);
        if(!matchedCqi) {
            for(auto& cqi: downloads) {
                if(cqi->getState() != ConnectionQueueItem::CONNECTING &&
                        cqi->getState() != ConnectionQueueItem::WAITING)
                    continue;
                // Nick fallback stays hub-scoped: never attach another hub's queue item.
                const string& hint = cqi->getUser().hint;
                if(!hint.empty() && !hubUrl.empty() && hint != hubUrl)
                    continue;
                if(nickMatchesHub(nick, cqi->getUser().user->getCID(), hubUrl)) {
                    matchedCqi = cqi;
                    break;
                }
            }
        }
        if(matchedCqi) {
            aSource->setUser(matchedCqi->getUser());
            aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
        }
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
