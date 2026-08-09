/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"
#include "OnlineUser.h"

#ifdef LUA_SCRIPT
#include "ScriptManager.h"
#endif

namespace dcpp {

#ifdef LUA_SCRIPT
struct ClientScriptInstance : public ScriptInstance {
    bool onHubFrameEnter(Client* aClient, const string& aLine);
    string formatChatMessage(const string& aLine);
};
#endif

/** Shared surface for a hub client or DHT peer (DC++ called this Client). */
class ClientBase
{
public:
    ClientBase() : type(DIRECT_CONNECT) { }

    enum P2PType { DIRECT_CONNECT, DHT };
    P2PType type;
    P2PType getType() const { return type; }
    virtual const string& getHubUrl() const = 0;
    virtual string getHubName() const = 0;
    virtual bool isOp() const = 0;
    virtual void connect(const OnlineUser& user, const string& token, bool reverseConnect = false, int secureMode = -1) = 0;
    virtual void privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson = false) = 0;
};

} // namespace dcpp
