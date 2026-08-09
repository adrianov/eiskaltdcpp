/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "compiler.h"

#include "BufferedSocketListener.h"
#include "ClientBase.h"
#include "ClientListener.h"
#include "forward.h"
#include "hub/HubConnectPace.h"
#include "hub/HubUserCounts.h"
#include "hub/SearchQueue.h"
#include "Socket.h"
#include "Speaker.h"
#include "TimerManager.h"
#include "NonCopyable.h"

namespace dcpp {

/** Yes, this should probably be called a Hub */
class Client : public ClientBase, public Speaker<ClientListener>, public BufferedSocketListener,
        protected TimerManagerListener
#ifdef LUA_SCRIPT
        , public ClientScriptInstance
#endif
        , private NonCopyable
{
public:
    typedef Client* Ptr;
    typedef list<Ptr> List;
    typedef List::iterator Iter;

    virtual void connect();
    virtual void disconnect(bool graceless);

    virtual void connect(const OnlineUser& user, const string& token, bool reverseConnect = false, int secureMode = -1) = 0;
    virtual void hubMessage(const string& aMessage, bool thirdPerson = false) = 0;
    virtual void privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson = false) = 0;
    virtual void sendUserCmd(const UserCommand& command, const ParamMap& params) = 0;

    uint64_t search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* owner);
    void cancelSearch(void* aOwner) { searchQueue.cancelSearch(aOwner); }
    void clearSearchQueue() { searchQueue.clear(); }

    virtual void password(const string& pwd) = 0;
    virtual void info(bool force) = 0;

    virtual size_t getUserCount() const = 0;
    virtual int64_t getAvailable() const = 0;
    static int getTotalCounts() { return counts.total(); }
    static string escape(string const& str) { return str; }
    static string getCounts() { return counts.format(); }

    virtual void emulateCommand(const string& cmd) = 0;
    virtual void send(const AdcCommand& command) = 0;

    bool isConnected() const { return state != STATE_DISCONNECTED; }
    bool isReady() const { return state != STATE_CONNECTING && state != STATE_DISCONNECTED; }
    bool isSecure() const;
    bool isTrusted() const;
    std::string getCipherName() const;
    vector<uint8_t> getKeyprint() const;

    bool isOp() const { return getMyIdentity().isOp(); }

    const string& getPort() const { return port; }
    const string& getAddress() const { return address; }
    const string& getIp() const { return ip; }
    string getIpPort() const { return getIp() + ':' + port; }
    string getLocalIp() const;

    StringMap& escapeParams(StringMap& sm);
    void setSearchInterval(uint32_t aInterval);
    uint32_t getSearchInterval() const { return searchQueue.interval; }

    /** False while this hub pauses further peer connects (CTM/RCM). */
    bool allowHubConnect() const { return connectPace.allow(); }
    /** Honor hub search/connect rate-limit text from chat or status. */
    void noteHubLimits(const string& message);

    void reconnect();
    void shutdown();
    bool isActive() const;
    bool handleRedirect(const string& targetUrl);
    void send(const string& aMessage) { send(aMessage.c_str(), aMessage.length()); }
    void send(const char* aMessage, size_t aLen);

    string getMyNick() const { return getMyIdentity().getNick(); }
    string getHubName() const { return getHubIdentity().getNick().empty() ? getHubUrl() : getHubIdentity().getNick(); }
    string getHubDescription() const { return getHubIdentity().getDescription(); }
    const string& getHubUrl() const { return hubUrl; }

    GETSET(Identity, myIdentity, MyIdentity);
    GETSET(Identity, hubIdentity, HubIdentity);
    Identity& getHubIdentity() { return hubIdentity; }

    GETSET(uint32_t, uniqueId, UniqueId);
    GETSET(string, defpassword, Password);
    GETSET(uint32_t, reconnDelay, ReconnDelay);
    GETSET(uint32_t, reconnAttempts, ReconnAttempts);
    GETSET(uint64_t, lastActivity, LastActivity);
    GETSET(bool, registered, Registered);
    GETSET(bool, autoReconnect, AutoReconnect);
    GETSET(bool, searchBlocked, SearchBlocked);
    GETSET(string, encoding, Encoding);
    GETSET(string, clientId, ClientId);
    GETSET(string, currentNick, CurrentNick);
    GETSET(string, currentDescription, CurrentDescription);

    string getFavIp() const { return externalIP; }
    void reloadSettings(bool updateNick);

protected:
    friend class ClientManager;
    Client(const string& hubURL, char separator, bool secure_, Socket::Protocol proto_);
    virtual ~Client();

    enum States {
        STATE_CONNECTING,
        STATE_PROTOCOL,
        STATE_IDENTIFY,
        STATE_VERIFY,
        STATE_NORMAL,
        STATE_DISCONNECTED
    } state;

    SearchQueue searchQueue;
    HubConnectPace connectPace;
    BufferedSocket* sock;

    static HubUserCounts counts;

    void updateCounts(bool aRemove);
    void updateActivity() { lastActivity = GET_TICK(); }
    void updated(OnlineUser& user);
    void updated(OnlineUserList& users);

    virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList) = 0;
    virtual string checkNick(const string& nick) = 0;

    bool tryAlternateNick();
    void scheduleReconnectBackoff();
    void onConnectFailed(const string& aLine);
    void storeHubNick();
    /** Toolbar/menu Reconnect: skip planned backoff once. */
    bool urgentReconnect;

    virtual void on(Second, uint64_t aTick) noexcept;
    virtual void on(Connecting) noexcept { fire(ClientListener::Connecting(), this); }
    virtual void on(Connected) noexcept;
    virtual void on(Line, const string& aLine) noexcept;
    virtual void on(Failed, const string&) noexcept;

private:
    enum CountType {
        COUNT_UNCOUNTED,
        COUNT_NORMAL,
        COUNT_REGISTERED,
        COUNT_OP
    };

    string hubUrl;
    string address;
    string ip;
    string localIp;
    string keyprint;
    string port;
    string externalIP;
    char separator;
    Socket::Protocol proto;
    bool secure;
    CountType countType;
};

} // namespace dcpp
