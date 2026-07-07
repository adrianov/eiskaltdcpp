/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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

#include <list>

#include "TimerManager.h"
#include "SettingsManager.h"
#include "forward.h"
#include "CriticalSection.h"
#include "Text.h"
#include "Client.h"

#ifdef LUA_SCRIPT
#include "ScriptManager.h"
#endif

namespace dcpp {

using std::list;

#ifdef LUA_SCRIPT
struct NmdcHubScriptInstance : public ScriptInstance {
    friend class ClientManager;
    bool onClientMessage(NmdcHub* aClient, const string& aLine);
};
#endif

class ClientManager;

#ifdef LUA_SCRIPT
class NmdcHub : public Client, private Flags ,public NmdcHubScriptInstance
#else
class NmdcHub : public Client, private Flags
#endif
{
public:
    using Client::send;
    using Client::connect;

    void onLine(const string& aLine) noexcept;
    virtual void connect(const OnlineUser& aUser, const string&, bool reverseConnect = false, int secureMode = -1) override;

    void hubMessage(const string& aMessage, bool /*thirdPerson*/ = false) override;
    void privateMessage(const OnlineUser& aUser, const string& aMessage, bool /*thirdPerson*/ = false) override;
    void sendUserCmd(const UserCommand& command, const ParamMap& params) override;
    void search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList) override;
    void password(const string& aPass) override { send("$MyPass " + fromUtf8(aPass) + "|"); }
    void info(bool force) override { myInfo(force); }

    size_t getUserCount() const override { Lock l(cs); return users.size(); }
    int64_t getAvailable() const override;

    static string escape(const string& str) { return validateMessage(str, false); }
    static string unescape(const string& str) { return validateMessage(str, true); }

    void emulateCommand(const string& cmd) override { onLine(cmd); }
    void send(const AdcCommand&) override { dcassert(0); }

    static string validateMessage(string tmp, bool reverse);

private:
    friend class ClientManager;
    enum SupportFlags {
        SUPPORTS_USERCOMMAND = 0x01,
        SUPPORTS_NOGETINFO = 0x02,
        SUPPORTS_USERIP2 = 0x04
    };

    mutable CriticalSection cs;

    unordered_map<string, OnlineUser*, CaseStringHash, CaseStringEq> users;

    int supportFlags;

    uint64_t lastUpdate;
    string lastMyInfoA;
    string lastMyInfoB;
    string lastMyInfoC;
    string lastMyInfoD;

    typedef list<pair<string, uint32_t> > FloodMap;
    typedef FloodMap::iterator FloodIter;
    FloodMap seekers;
    FloodMap flooders;

    NmdcHub(const string& aHubURL, bool secure);
    virtual ~NmdcHub();

    void clearUsers();

    OnlineUser& getUser(const string& aNick);
    OnlineUser* findUser(const string& aNick);
    void putUser(const string& aNick);

    string toUtf8(const string& str) const { return Text::validateUtf8(str) ? str : Text::toUtf8(str, getEncoding()); }
    string fromUtf8(const string& str) const { return Text::fromUtf8(str, getEncoding()); }
    void privateMessage(const string& nick, const string& aMessage);
    void validateNick(const string& aNick) { send("$ValidateNick " + fromUtf8(aNick) + "|"); }
    void key(const string& aKey) { send("$Key " + aKey + "|"); }
    void version() { send("$Version 1,0091|"); }
    void getNickList() { send("$GetNickList|"); }
    void connectToMe(const OnlineUser& aUser, int secureMode = -1);
    void revConnectToMe(const OnlineUser& aUser);
    void myInfo(bool alwaysSend);
    void supports(const StringList& feat);
    void clearFlooders(uint64_t tick);
    void stopInfectedConnect(const string& message, const string& aNick = Util::emptyString);

    void updateFromTag(Identity& id, const string& tag);
    void findTagInMyINFO(Identity& id, const string& param, size_t start);

    string checkNick(const string& aNick) override;

    // TimerManagerListener
    void on(Second, uint64_t aTick) noexcept override;

    void on(Connected) noexcept override;
    void on(Line, const string& l) noexcept override;
    void on(Failed, const string&) noexcept override;

};

} // namespace dcpp
