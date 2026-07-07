/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"

#include "ConnectionManager.h"

#include "PeerConnectLog.h"

#include "Client.h"
#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "DownloadManager.h"
#include "LogManager.h"
#include "QueueManager.h"
#include "UploadManager.h"
#include "UserConnection.h"

namespace dcpp {

ConnectionManager::ConnectionManager() :
    floodCounter(0),
    server(0),
    secureServer(0),
    shuttingDown(false)
{
    TimerManager::getInstance()->addListener(this);

    features = {
        UserConnection::FEATURE_MINISLOTS,
        UserConnection::FEATURE_XML_BZLIST,
        UserConnection::FEATURE_ADCGET,
        UserConnection::FEATURE_TTHL,
        UserConnection::FEATURE_TTHF
    };

    adcFeatures = {
        "AD" + UserConnection::FEATURE_ADC_BAS0,
        "AD" + UserConnection::FEATURE_ADC_BASE,
        "AD" + UserConnection::FEATURE_ADC_TIGR,
        "AD" + UserConnection::FEATURE_ADC_BZIP
    };
}

void ConnectionManager::listen() {
    disconnect();

    server = new Server(false, Util::toString(SETTING(TCP_PORT)), SETTING(BIND_ADDRESS));

    if(!CryptoManager::getInstance()->TLSOk()) {
        dcdebug("Skipping secure port: %d\n", SETTING(TLS_PORT));
        return;
    }

    secureServer = new Server(true, Util::toString(SETTING(TLS_PORT)), SETTING(BIND_ADDRESS));
}

ConnectionQueueItem::ConnectionQueueItem(const HintedUser &aUser, bool aDownload) :
    token(Util::toString(Util::rand())),
    lastAttempt(0),
    errors(0),
    connectAttempts(0),
    state(WAITING),
    download(aDownload),
    user(aUser)
{
}

UserConnection* ConnectionManager::getConnection(bool aNmdc, bool secure) noexcept {
    UserConnection* uc = new UserConnection(secure);
    uc->addListener(this);
    {
        Lock l(cs);
        userConnections.push_back(uc);
    }
    if(aNmdc)
        uc->setFlag(UserConnection::FLAG_NMDC);
    return uc;
}

void ConnectionManager::putConnection(UserConnection* aConn) {
    aConn->removeListener(this);
    aConn->disconnect();

    Lock l(cs);
    userConnections.erase(remove(userConnections.begin(), userConnections.end(), aConn), userConnections.end());
}

void ConnectionManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
    Lock l(cs);

    for(auto& j : userConnections) {
        if((j->getLastActivity() + 180*1000) < aTick) {
            j->disconnect(true);
        }
    }
}

const string& ConnectionManager::getPort() const {
    return server ? server->getPort() : Util::emptyString;
}

const string& ConnectionManager::getSecurePort() const {
    return secureServer ? secureServer->getPort() : Util::emptyString;
}

void ConnectionManager::nmdcConnect(const string& aServer, const string& aPort, const string& aNick, const string& hubUrl, const string& encoding, bool secure) {
    nmdcConnect(aServer, aPort, Util::emptyString, BufferedSocket::NAT_NONE, aNick, hubUrl, encoding, secure);
}

void ConnectionManager::nmdcConnect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding, bool secure) {
    if(shuttingDown)
        return;

    if (checkHubCCBlock(aServer, aPort, hubUrl))
        return;

    UserConnection* uc = getConnection(true, secure);
    uc->setToken(aNick);
    uc->setHubUrl(hubUrl);
    uc->setEncoding(encoding);
    uc->setState(UserConnection::STATE_CONNECT);
    uc->setFlag(UserConnection::FLAG_NMDC);
    PeerConnectLog::tcpOut(aNick, aServer, aPort, secure, "NMDC");
    try {
        uc->connect(aServer, aPort, localPort, natRole);
    } catch(const Exception& e) {
        PeerConnectLog::tcpFail(aNick, aServer, aPort, e.getError());
        LogManager::getInstance()->message(str(F_("NMDC connect to %1%:%2% failed: %3%") % aServer % aPort % e.getError()));
        putConnection(uc);
        delete uc;
    }
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string &aPort, const string& aToken, bool secure) {
    adcConnect(aUser, aPort, Util::emptyString, BufferedSocket::NAT_NONE, aToken, secure);
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string &aPort, const string &localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure) {
    if(shuttingDown)
        return;

    UserConnection* uc = getConnection(false, secure);
    uc->setToken(aToken);
    uc->setEncoding(Text::utf8);
    uc->setState(UserConnection::STATE_CONNECT);
#ifdef WITH_DHT
    uc->setHubUrl(aUser.getClient().getType() == Client::DHT ? "DHT" : aUser.getClient().getHubUrl());
#else
    uc->setHubUrl(aUser.getClient().getHubUrl());
#endif
    if(aUser.getIdentity().isOp()) {
        uc->setFlag(UserConnection::FLAG_OP);
    }
    PeerConnectLog::tcpOut(aUser.getIdentity().getNick(), aUser.getIdentity().getIp(), aPort, secure, "ADC");
    try {
        uc->connect(aUser.getIdentity().getIp(), aPort, localPort, natRole);
    } catch(const Exception& e) {
        PeerConnectLog::tcpFail(aUser.getIdentity().getNick(), aUser.getIdentity().getIp(), aPort, e.getError());
        putConnection(uc);
        delete uc;
    }
}

void ConnectionManager::disconnect() noexcept {
    delete server;
    delete secureServer;
    server = nullptr;
    secureServer = nullptr;
}

void ConnectionManager::on(AdcCommand::SUP, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_SUPNICK) {
        // Already got this once, ignore...@todo fix support updates
        dcdebug("CM::onSUP %p sent sup twice\n", (void*)aSource);
        return;
    }

    bool baseOk = false;

    for(auto& i: cmd.getParameters()) {
        if(i.compare(0, 2, "AD") == 0) {
            string feat = i.substr(2);
            if(feat == UserConnection::FEATURE_ADC_BASE || feat == UserConnection::FEATURE_ADC_BAS0) {
                baseOk = true;
                // ADC clients must support all these...
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
                aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
                // For compatibility with older clients...
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
        aSource->inf(false);
    } else {
        aSource->inf(true);
    }
    aSource->setState(UserConnection::STATE_INF);
}

void ConnectionManager::on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept {

}

void ConnectionManager::on(UserConnectionListener::Connected, UserConnection* aSource) noexcept {
    // incorrect check because aSource->getUser().get() == nullptr
    if(aSource->isSecure() && !aSource->isTrusted() && !BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS)) {
        putConnection(aSource);
        //        QueueManager::getInstance()->removeSource(aSource->getUser(), QueueItem::Source::FLAG_UNTRUSTED);
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

void ConnectionManager::on(UserConnectionListener::MyNick, UserConnection* aSource, const string& aNick) noexcept {
    if(aSource->getState() != UserConnection::STATE_SUPNICK) {
        // Already got this once, ignore...
        dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
        return;
    }

    dcassert(!aNick.empty());
    dcdebug("ConnectionManager::onMyNick %p, %s\n", (void*)aSource, aNick.c_str());

    if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
        // Try to guess where this came from...
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

    // First, we try looking in the pending downloads...hopefully it's one of them...
    {
        Lock l(cs);
        for(auto& cqi: downloads) {
            cqi->setErrors(0);
            if((cqi->getState() == ConnectionQueueItem::CONNECTING || cqi->getState() == ConnectionQueueItem::WAITING) &&
                    cqi->getUser().user->getCID() == cid)
            {
                aSource->setUser(cqi->getUser());
                // Indicate that we're interested in this file...
                aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
                break;
            }
        }
    }

    if(!aSource->getUser()) {
        // Make sure we know who it is, i e that he/she is connected...

        aSource->setUser(ClientManager::getInstance()->findUser(cid));
        if(!aSource->getUser() || !ClientManager::getInstance()->isOnline(aSource->getUser())) {
            dcdebug("CM::onMyNick Incoming connection from unknown user %s\n", nick.c_str());
            PeerConnectLog::incomingReject(nick, _("user not online on hub"));
            putConnection(aSource);
            return;
        }
        // We don't need this connection for downloading...make it an upload connection instead...
        aSource->setFlag(UserConnection::FLAG_UPLOAD);
    }

    if(ClientManager::getInstance()->isOp(aSource->getUser(), aSource->getHubUrl()))
        aSource->setFlag(UserConnection::FLAG_OP);

    ClientManager::getInstance()->setIPUser(aSource->getUser(), aSource->getRemoteIp());

    if( aSource->isSet(UserConnection::FLAG_INCOMING) ) {
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

    if( CryptoManager::getInstance()->isExtended(aLock) ) {
        StringList defFeatures = features;
        if(BOOLSETTING(COMPRESS_TRANSFERS)) {
            defFeatures.push_back(UserConnection::FEATURE_ZLIB_GET);
        }

        aSource->supports(defFeatures);
    }

    aSource->setState(UserConnection::STATE_DIRECTION);
    aSource->direction(aSource->getDirectionString(), aSource->getNumber());
    aSource->key(CryptoManager::getInstance()->makeKey(aLock));
}

void ConnectionManager::on(UserConnectionListener::Direction, UserConnection* aSource, const string& dir, const string& num) noexcept {
    if(aSource->getState() != UserConnection::STATE_DIRECTION) {
        dcdebug("CM::onDirection %p received direction twice, ignoring\n", (void*)aSource);
        return;
    }

    dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));
    if(dir == "Upload") {
        // Fine, the other fellow want's to send us data...make sure we really want that...
        if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
            // Huh? Strange...disconnect...
            putConnection(aSource);
            return;
        }
    } else {
        if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
            int number = Util::toInt(num);
            // Damn, both want to download...the one with the highest number wins...
            if(aSource->getNumber() < number) {
                // Damn! We lost!
                aSource->unsetFlag(UserConnection::FLAG_DOWNLOAD);
                aSource->setFlag(UserConnection::FLAG_UPLOAD);
            } else if(aSource->getNumber() == number) {
                putConnection(aSource);
                return;
            }
        }
    }

    dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));

    aSource->setState(UserConnection::STATE_KEY);
}

void ConnectionManager::addDownloadConnection(UserConnection* uc) {
    bool addConn = false;
    {
        Lock l(cs);

        auto i = find(downloads.begin(), downloads.end(), uc->getUser());
        if(i != downloads.end()) {
            auto& cqi = *i;
            if(cqi->getState() == ConnectionQueueItem::WAITING || cqi->getState() == ConnectionQueueItem::CONNECTING) {
                cqi->setState(ConnectionQueueItem::ACTIVE);
                uc->setFlag(UserConnection::FLAG_ASSOCIATED);

                fire(ConnectionManagerListener::Connected(), cqi);

                PeerConnectLog::connected(cqi->getUser(), true);
                dcdebug("ConnectionManager::addDownloadConnection, leaving to downloadmanager\n");
                addConn = true;
            }
        }
    }

    if(addConn) {
        DownloadManager::getInstance()->addConnection(uc);
    } else {
        putConnection(uc);
    }
}

void ConnectionManager::addUploadConnection(UserConnection* uc) {
    dcassert(uc->isSet(UserConnection::FLAG_UPLOAD));

    bool addConn = false;
    {
        Lock l(cs);

        auto i = find(uploads.begin(), uploads.end(), uc->getUser());
        if(i == uploads.end()) {
            ConnectionQueueItem* cqi = getCQI(uc->getHintedUser(), false);

            cqi->setState(ConnectionQueueItem::ACTIVE);
            uc->setFlag(UserConnection::FLAG_ASSOCIATED);

            fire(ConnectionManagerListener::Connected(), cqi);

            PeerConnectLog::connected(cqi->getUser(), false);
            dcdebug("ConnectionManager::addUploadConnection, leaving to uploadmanager\n");
            addConn = true;
        }
    }

    if(addConn) {
        UploadManager::getInstance()->addConnection(uc);
    } else {
        putConnection(uc);
    }
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

void ConnectionManager::on(AdcCommand::INF, UserConnection* aSource, const AdcCommand& cmd) noexcept {
    if(aSource->getState() != UserConnection::STATE_INF) {
        aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Expecting INF"));
        aSource->disconnect();
        return;
    }

    // Leaks CSUPs, other client's CINF, and ADC connection's presence. Allows removing
    // user from queue by waiting long enough for aSource->getUser() to function.
    if(SETTING(REQUIRE_TLS) && !aSource->isSet(UserConnection::FLAG_NMDC) && !aSource->isSecure()) {
        putConnection(aSource);
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

    if(!aSource->getUser()) {
        aSource->setUser(ClientManager::getInstance()->findUser(CID(cid)));

        if(!aSource->getUser()) {
            PeerConnectLog::incomingReject(cid, _("ADC INF ID not found on hub"));
            aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID: user not found").addParam("FB", "ID"));
            putConnection(aSource);
            return;
        }
    }

    // without a valid KeyPrint this degrades into normal turst check
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
            putConnection(aSource);
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
        /** @todo check tokens for upload connections */
    }

    if(down) {
        aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
        addDownloadConnection(aSource);
    } else {
        aSource->setFlag(UserConnection::FLAG_UPLOAD);
        addUploadConnection(aSource);
    }
}

void ConnectionManager::force(const UserPtr& aUser) {
    Lock l(cs);

    auto i = find(downloads.begin(), downloads.end(), aUser);
    if(i != downloads.end()) {
        (*i)->setLastAttempt(0);
    }
}

void ConnectionManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) noexcept {
    failed(aSource, aError, false);
}

void ConnectionManager::on(UserConnectionListener::ProtocolError, UserConnection* aSource, const string& aError) noexcept {
    if(aError.compare(0, 7, "CTM2HUB", 7) == 0) {
        {
            Lock l(cs);
            hubsBlockingCC.insert(Text::toLower(aSource->getRemoteIp()));
        }

        string aServerPort = aSource->getRemoteIp() + ":" + aSource->getPort();
        LogManager::getInstance()->message(str(F_("Blocking '%1%', potential DDoS detected (originating hub '%2%')") % aServerPort % aSource->getHubUrl() ));
    }

    failed(aSource, aError, true);
}

void ConnectionManager::disconnect(const UserPtr& user) {
    Lock l(cs);
    for(auto uc: userConnections) {
        if(uc->getUser() == user)
            uc->disconnect(true);
    }
}

void ConnectionManager::disconnect(const UserPtr& user, int isDownload) {
    Lock l(cs);
    for(auto uc: userConnections) {
        if(uc->getUser() == user && uc->isSet(isDownload ? UserConnection::FLAG_DOWNLOAD : UserConnection::FLAG_UPLOAD)) {
            uc->disconnect(true);
            break;
        }
    }
}

void ConnectionManager::blockRetry(const UserPtr& user) {
    Lock l(cs);
    auto i = find(downloads.begin(), downloads.end(), user);
    if(i != downloads.end()) {
        // errors is int; -1 blocks reconnect (see ConnectionQueueItem::errors)
        (*i)->setErrors(-1);
        if((*i)->getState() == ConnectionQueueItem::CONNECTING)
            (*i)->setState(ConnectionQueueItem::WAITING);
    }
    disconnect(user);
}

void ConnectionManager::shutdown() {
    TimerManager::getInstance()->removeListener(this);
    shuttingDown = true;
    disconnect();
    {
        Lock l(cs);
        for(auto j: userConnections) {
            j->disconnect(true);
        }
    }
    // Wait until all connections have died out...
    while(true) {
        {
            Lock l(cs);
            if(userConnections.empty()) {
                break;
            }
        }
        Thread::sleep(50);
    }
}

// UserConnectionListener
void ConnectionManager::on(UserConnectionListener::Supports, UserConnection* conn, const StringList& feat) noexcept {
    for(auto& i: feat) {
        if(i == UserConnection::FEATURE_MINISLOTS) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
        } else if(i == UserConnection::FEATURE_XML_BZLIST) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
        } else if(i == UserConnection::FEATURE_ADCGET) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
        } else if(i == UserConnection::FEATURE_ZLIB_GET) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
        } else if(i == UserConnection::FEATURE_TTHL) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
        } else if(i == UserConnection::FEATURE_TTHF) {
            conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
        }
    }
}

} // namespace dcpp
