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
#include "PeerConnectTls.h"

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
    slotWaits(0),
    state(WAITING),
    download(aDownload),
    secureMode(PeerConnectTls::learnedTlsRequired(aUser.user) ? PeerConnectTls::TLS : PeerConnectTls::AUTO),
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

void ConnectionManager::adcExpect(const string& token, const UserPtr& user) {
    if(token.empty() || !user)
        return;
    Lock l(cs);
    adcExpected.emplace(token, make_pair(user, GET_TICK()));
}

bool ConnectionManager::consumeAdc(const string& token, const UserPtr& user) {
    Lock l(cs);
    auto range = adcExpected.equal_range(token);
    for(auto i = range.first; i != range.second; ++i) {
        if(i->second.first == user) {
            adcExpected.erase(i);
            return true;
        }
    }
    return false;
}

void ConnectionManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
    Lock l(cs);

    for(auto& j : userConnections) {
        if((j->getLastActivity() + 180*1000) < aTick) {
            j->disconnect(true);
        }
    }
    for(auto i = adcExpected.begin(); i != adcExpected.end();) {
        if(i->second.second + 180*1000 < aTick)
            i = adcExpected.erase(i);
        else
            ++i;
    }
}

const string& ConnectionManager::getPort() const {
    return server ? server->getPort() : Util::emptyString;
}

const string& ConnectionManager::getSecurePort() const {
    return secureServer ? secureServer->getPort() : Util::emptyString;
}

void ConnectionManager::disconnect() noexcept {
    delete server;
    delete secureServer;
    server = nullptr;
    secureServer = nullptr;
}

void ConnectionManager::on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept {
}

void ConnectionManager::force(const UserPtr& aUser) {
    Lock l(cs);
    clearConnectCooldown(aUser);
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

} // namespace dcpp
