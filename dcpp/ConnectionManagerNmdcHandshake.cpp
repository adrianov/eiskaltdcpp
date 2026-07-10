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

#include "CryptoManager.h"
#include "PeerConnectLog.h"
#include "UserConnection.h"

namespace dcpp {

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
                // Peer won the download tie — we would become their upload source.
                // If we are already in MaxedOut/slot-wait backoff for this user, drop the
                // socket instead of churning upload CQIs and "Connected (upload)" noise.
                {
                    Lock l(cs);
                    auto* cqi = findDownloadCqi(aSource->getHintedUser());
                    if(cqi && slotWaitActive(cqi)) {
                        putConnection(aSource);
                        return;
                    }
                }
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
