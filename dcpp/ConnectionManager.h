/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
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

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "BufferedSocket.h"
#include "ConnectionManagerListener.h"
#include "ConnectionQueueItem.h"
#include "CriticalSection.h"
#include "NonCopyable.h"
#include "Singleton.h"
#include "TimerManager.h"
#include "UserConnectionListener.h"

namespace dcpp {

using std::unique_ptr;
using std::unordered_map;
using std::vector;

class SocketException;

class ConnectionManager : public Speaker<ConnectionManagerListener>,
        public UserConnectionListener, TimerManagerListener,
        public Singleton<ConnectionManager>
{
public:
    void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl) {
        expectedConnections.add(aNick, aMyNick, aHubUrl);
    }

    void nmdcConnect(const string& aServer, const string& aPort, const string& aMyNick, const string& hubUrl, const string& encoding, bool secure);
    void nmdcConnect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding, bool secure);
    void adcConnect(const OnlineUser& aUser, const string& aPort, const string& aToken, bool secure);
    void adcConnect(const OnlineUser& aUser, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure);

    void getDownloadConnection(const HintedUser& aUser);
    void force(const UserPtr& aUser);
    void onUpnpReady();

    void disconnect(const UserPtr& user); // disconnect all transfers for the user
    void disconnect(const UserPtr& user, int isDownload);
    void blockRetry(const UserPtr& user);

    void shutdown();

    /** Find a suitable port to listen on, and start doing it */
    void listen();
    void disconnect() noexcept;

    const string& getPort() const;
    const string& getSecurePort() const;

private:

    class Server : public Thread {
    public:
        Server(const bool secure_, const std::string& port, const string& ip = "0.0.0.0");
        virtual ~Server() { die = true; join(); }

        const string& getPort() const { return port; }

    private:
        virtual int run() noexcept;

        Socket sock;
        string port;
        string ip;
        bool secure;
        bool die;
    };

    friend class Server;

    mutable CriticalSection cs;

    /** All ConnectionQueueItems */
    ConnectionQueueItem::List downloads;
    ConnectionQueueItem::List uploads;

    /** All active connections */
    UserConnectionList userConnections;

    StringList features;
    StringList adcFeatures;

    ExpectedMap expectedConnections;

    uint32_t floodCounter;
    unordered_set<string> hubsBlockingCC;

    Server* server;
    Server* secureServer;

    bool shuttingDown;

    friend class Singleton<ConnectionManager>;
    ConnectionManager();

    virtual ~ConnectionManager() { shutdown(); }

    UserConnection* getConnection(bool aNmdc, bool secure) noexcept;
    void putConnection(UserConnection* aConn);

    void rejectConnection(UserConnection* aConn, const string& reason);

    void addDownloadConnection(UserConnection* uc);
    void addUploadConnection(UserConnection* uc);

    ConnectionQueueItem* getCQI(const HintedUser& user, bool download);
    void putCQI(ConnectionQueueItem* cqi);

    bool checkKeyprint(UserConnection *aSource);

    void accept(const Socket& sock, bool secure) noexcept;

    void failed(UserConnection* aSource, const string& aError, bool protocolError);
    void failDownloadQueue(ConnectionQueueItem* dlCqi, UserConnection* aSource, const string& aError, bool protocolError);
    void markQueueGiveUp(ConnectionQueueItem* cqi, int attempts);

    bool checkHubCCBlock(const string& aServer, const string& aPort, const string& aHubUrl);

    // UserConnectionListener
    virtual void on(Connected, UserConnection*) noexcept;
    virtual void on(Failed, UserConnection*, const string&) noexcept;
    virtual void on(ProtocolError, UserConnection*, const string&) noexcept;
    virtual void on(CLock, UserConnection*, const string&, const string&) noexcept;
    virtual void on(Key, UserConnection*, const string&) noexcept;
    virtual void on(Direction, UserConnection*, const string&, const string&) noexcept;
    virtual void on(MyNick, UserConnection*, const string&) noexcept;
    virtual void on(Supports, UserConnection*, const StringList&) noexcept;

    virtual void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) noexcept;
    virtual void on(AdcCommand::INF, UserConnection*, const AdcCommand&) noexcept;
    virtual void on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept;

    // TimerManagerListener
    virtual void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
    virtual void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;

};

} // namespace dcpp
