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
#include "PeerConnectLog.h"
#include "QueueManager.h"
#include "UploadManager.h"
#include "UserConnection.h"

namespace dcpp {

void ConnectionManager::on(AdcCommand::SUP, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_SUPNICK) {
        dcdebug("CM::onSUP %p sent sup twice\n", (void*)aSource);
        return;
    }

    bool baseOk = false;

    for(auto& i: cmd.getParameters()) {
        if(i.compare(0, 2, "AD") == 0) {
            string feat = i.substr(2);
            if(feat == UserConnection::FEATURE_ADC_BASE || feat == UserConnection::FEATURE_ADC_BAS0) {
                baseOk = true;
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
            } else if(feat == UserConnection::FEATURE_ZLIB_GET) {
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
            } else if(feat == UserConnection::FEATURE_ADC_BZIP) {
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
            }
        }
    }

    if(!baseOk) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Invalid SUP"));
        aSource->disconnect();
        return;
    }

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        StringList defFeatures = adcFeatures;
        if(BOOLSETTING(COMPRESS_TRANSFERS)) {
            defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
        }
        aSource->sup(defFeatures);
    } else {
        aSource->inf(true);
    }
    aSource->setState(UserConnection::STATE_INF);
}

void ConnectionManager::on(UserConnectionListener::Connected, UserConnection* aSource) noexcept {
    if(aSource->isSecure() && !aSource->isTrusted() && !BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS)) {
        rejectConnection(aSource, _("untrusted TLS certificate (ALLOW_UNTRUSTED_CLIENTS disabled)"));
        return;
    }

    dcassert(aSource->getState() == UserConnection::STATE_CONNECT);
    if(aSource->isSet(UserConnection::FLAG_NMDC)) {
        aSource->myNick(aSource->getToken());
        aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk() + "Ref=" + aSource->getHubUrl());
    } else {
        StringList defFeatures = adcFeatures;
        if(BOOLSETTING(COMPRESS_TRANSFERS)) {
            defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
        }
        aSource->sup(defFeatures);
        aSource->send(AdcCommand(AdcCommand::SEV_SUCCESS, AdcCommand::SUCCESS, Util::emptyString).addParam("RF", aSource->getHubUrl()));
    }
    aSource->setState(UserConnection::STATE_SUPNICK);
}

void ConnectionManager::on(AdcCommand::INF, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_INF) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Expecting INF"));
        aSource->disconnect();
        return;
    }

    if(SETTING(REQUIRE_TLS) && !aSource->isSet(UserConnection::FLAG_NMDC) && !aSource->isSecure()) {
        rejectConnection(aSource, _("ADC connection not encrypted (REQUIRE_TLS enabled)"));
        QueueManager::getInstance()->removeSource(aSource->getUser(), QueueItem::Source::FLAG_UNENCRYPTED);
        return;
    }

    string cid;
    if(!cmd.getParam("ID", 0, cid)) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID missing").addParam("FM", "ID"));
        dcdebug("CM::onINF missing ID\n");
        aSource->disconnect();
        return;
    }
    if(cid.size() != 39) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID invalid").addParam("FB", "ID"));
        aSource->disconnect();
        return;
    }

    const CID peerCid(cid);
    if(aSource->getUser() && !(aSource->getUser()->getCID() == peerCid)) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID mismatch").addParam("FB", "ID"));
        rejectConnection(aSource, _("ADC peer CID does not match requested user"));
        return;
    }
    if(!aSource->getUser()) {
        aSource->setUser(ClientManager::getInstance()->findUser(peerCid));

        if(!aSource->getUser()) {
            PeerConnectLog::incomingReject(cid, _("ADC INF ID not found on hub"));
            aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID: user not found").addParam("FB", "ID"));
            putConnection(aSource);
            return;
        }
    }

    if(!checkKeyprint(aSource)) {
        PeerConnectLog::skip(ClientManager::getInstance()->getNickOrCid(aSource->getHintedUser()),
            aSource->getHubUrl(), _("TLS keyprint mismatch"));
        QueueManager::getInstance()->removeSource(aSource->getUser(), QueueItem::Source::FLAG_UNTRUSTED);
        putConnection(aSource);
        return;
    }

    string token;
    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        if(!cmd.getParam("TO", 0, token)) {
            aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF TO missing").addParam("FM", "TO"));
            rejectConnection(aSource, _("ADC incoming INF missing TO field"));
            return;
        }
    } else {
        token = aSource->getToken();
    }

    bool down = false;
    {
        Lock l(cs);
        auto i = find(downloads.begin(), downloads.end(), aSource->getUser());

        if(i != downloads.end()) {
            (*i)->setErrors(0);

            const string& to = (*i)->getToken();

            if(to == token) {
                down = true;
            }
        }
    }

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        const bool expected = consumeAdc(token, aSource->getUser());
        if(!down && !expected) {
            rejectConnection(aSource, _("unexpected ADC connection token"));
            return;
        }
        // The connecting party sends INF first; answer after validating its identity and token.
        aSource->inf(false);
    }

    if(down) {
        aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
        addDownloadConnection(aSource);
    } else {
        aSource->setFlag(UserConnection::FLAG_UPLOAD);
        addUploadConnection(aSource);
    }
}

} // namespace dcpp
