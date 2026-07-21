/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "CID.h"
#include "BufferedSocket.h"
#include "ConnectionManagerListener.h"
#include "ConnectionQueueItem.h"
#include "CriticalSection.h"
#include "ExpectedMap.h"
#include "NonCopyable.h"
#include "Singleton.h"
#include "TimerManager.h"
#include "UserConnectionListener.h"

namespace dcpp {

using std::unique_ptr;
using std::unordered_map;
using std::vector;

using QueuedDownloadUsers = unordered_set<CID>;

class SocketException;

class ConnectionManager : public Speaker<ConnectionManagerListener>,
        public UserConnectionListener, TimerManagerListener,
        public Singleton<ConnectionManager>
{
public:
    void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl);

    void nmdcConnect(const string& aServer, const string& aPort, const string& aMyNick, const string& hubUrl, const string& encoding, bool secure);
    void nmdcConnect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding, bool secure);
    void adcConnect(const OnlineUser& aUser, const string& aPort, const string& aToken, bool secure);
    void adcConnect(const OnlineUser& aUser, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure);
    void adcExpect(const string& token, const UserPtr& user);

    void getDownloadConnection(const HintedUser& aUser);
    void force(const UserPtr& aUser);
    void onUpnpReady();

    /** False while a short CTM latch is active or a download CQI is CONNECTING. */
    bool allowOutgoingConnect(const UserPtr& user) const;
    /** True when a download CQI exists for the user (connecting, waiting, or active). */
    bool isQueuedForDownload(const UserPtr& user) const;
    /** Snapshot of users with a live download CQI (caller must not hold QueueManager::cs). */
    QueuedDownloadUsers queuedDownloadUsers() const;
    /** Arm short CTM/RCM latch (RevConnect / upload-notify anti-spam). */
    void noteOutgoingConnect(const UserPtr& user);
    /** Clear CTM latch. */
    void clearOutgoingConnect(const UserPtr& user);
    /** Peer granted a download slot: mark CQI and clear CTM latch. */
    void clearOutgoingStrikes(const UserPtr& user);
    /** MaxedOut: next close should use slot-wait backoff, not between-files retry. */
    void forgetDownloadSlot(const UserPtr& user);

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
    std::unordered_multimap<string, std::pair<UserPtr, uint64_t>> adcExpected;

    /** Short CTM/RCM latch until tick (list-peer-keyed; not download retry policy). */
    mutable CriticalSection cooldownCs;
    unordered_map<string, uint64_t> ctmLatch;

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
    ConnectionQueueItem* findDownloadCqi(const HintedUser& user);
    /** True when another hub identity for the same peer is already connecting. */
    bool peerConnectInFlight(const HintedUser& user) const;
    ConnectionQueueItem* findDownloadCqiForHub(const string& hubUrl, const CID& wireCid) const;
    bool slotWaitActive(const ConnectionQueueItem* cqi) const;
    bool queueBackoffActive(const ConnectionQueueItem* cqi) const;
    static void mergeQueueState(ConnectionQueueItem* keep, const ConnectionQueueItem* other);

    bool checkKeyprint(UserConnection *aSource);
    bool consumeAdc(const string& token, const UserPtr& user);

    void accept(const Socket& sock, bool secure) noexcept;

    void failed(UserConnection* aSource, const string& aError, bool protocolError);
    void failDownloadQueue(ConnectionQueueItem* dlCqi, UserConnection* aSource, const string& aError, bool protocolError);
    void markQueueGiveUp(ConnectionQueueItem* cqi, int attempts, bool slotWait);
    void reviveDownloadQueue(ConnectionQueueItem* cqi, bool forced = false);
    /** CONNECTING timed out. True → drop CQI; peer is unreachable (caller removes queue sources). */
    bool onDownloadConnectTimeout(ConnectionQueueItem* cqi);
    /** All online hubs timed out with no IP/slot-wait — log, clear session, caller removes CQI/sources. */
    bool dropUnreachableDownload(ConnectionQueueItem* cqi);

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
