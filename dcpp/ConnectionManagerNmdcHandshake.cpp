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
#include "DownloadManager.h"
#include "Encoder.h"
#include "LogManager.h"
#include "PeerConnectLog.h"
#include "QueueManager.h"
#include "UploadManager.h"
#include "UserConnection.h"
#include "format.h"

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

    string nick = Text::toUtf8(aNick, aSource->getEncoding());
    CID cid = ClientManager::getInstance()->makeCid(nick, aSource->getHubUrl());

    {
        Lock l(cs);
        for(auto& cqi: downloads) {
            if((cqi->getState() == ConnectionQueueItem::CONNECTING || cqi->getState() == ConnectionQueueItem::WAITING) &&
                    cqi->getUser().user->getCID() == cid)
            {
                cqi->setErrors(0);
                aSource->setUser(cqi->getUser());
                aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
                break;
            }
        }
    }

    if(!aSource->getUser()) {
        aSource->setUser(ClientManager::getInstance()->findUser(cid));
        if(!aSource->getUser() || !ClientManager::getInstance()->isOnline(aSource->getUser())) {
            dcdebug("CM::onMyNick Incoming connection from unknown user %s\n", nick.c_str());
            PeerConnectLog::incomingReject(nick, _("user not online on hub"));
            putConnection(aSource);
            return;
        }
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

void ConnectionManager::on(UserConnectionListener::CLock, UserConnection* aSource, const string& aLock, const string& aPk) noexcept {
    (void)aPk;

    if(aSource->getState() != UserConnection::STATE_LOCK) {
        dcdebug("CM::onLock %p received lock twice, ignoring\n", (void*)aSource);
        return;
    }

    if(CryptoManager::getInstance()->isExtended(aLock)) {
        StringList defFeatures = features;
        if(BOOLSETTING(COMPRESS_TRANSFERS)) {
            defFeatures.push_back(UserConnection::FEATURE_ZLIB_GET);
        }

        aSource->supports(defFeatures);
    }

    aSource->setPeerDirection(Util::emptyString);
    aSource->setPeerDirectionNum(-1);
    aSource->setState(UserConnection::STATE_DIRECTION);
    aSource->direction(aSource->getDirectionString(), aSource->getNumber());
    aSource->key(CryptoManager::getInstance()->makeKey(aLock));
}

void ConnectionManager::on(UserConnectionListener::Direction, UserConnection* aSource, const string& dir, const string& num) noexcept {
    aSource->setPeerDirection(dir);
    aSource->setPeerDirectionNum(Util::toInt(num));
    if(aSource->getState() != UserConnection::STATE_DIRECTION) {
        dcdebug("CM::onDirection %p received direction twice, ignoring\n", (void*)aSource);
        return;
    }

    dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));
    if(dir == "Upload") {
        if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
            rejectConnection(aSource, _("direction conflict (both sides upload)"));
            return;
        }
    } else {
        if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
            int number = Util::toInt(num);
            if(aSource->getNumber() < number) {
                aSource->unsetFlag(UserConnection::FLAG_DOWNLOAD);
                aSource->setFlag(UserConnection::FLAG_UPLOAD);
            } else if(aSource->getNumber() == number) {
                rejectConnection(aSource, _("direction tie (same number, both download)"));
                return;
            }
        }
    }

    dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));

    aSource->setState(UserConnection::STATE_KEY);
}

void ConnectionManager::on(UserConnectionListener::Key, UserConnection* aSource, const string&/* aKey*/) noexcept {
    if(aSource->getState() != UserConnection::STATE_KEY) {
        dcdebug("CM::onKey Bad state, ignoring");
        return;
    }

    if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
        addDownloadConnection(aSource);
    } else {
        addUploadConnection(aSource);
    }
}

} // namespace dcpp
