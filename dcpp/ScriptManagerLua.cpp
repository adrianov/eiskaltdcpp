/*
 * Copyright (C) 2010 cologic, ne5@parsoma.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LuaManager bindings for ScriptManager.
 */

#include "stdinc.h"

#include "ScriptManager.h"
#include "Util.h"
#include "StringTokenizer.h"
#include "Client.h"
#include "UserConnection.h"
#include "ClientManager.h"
#include "NmdcHub.h"
#include "AdcHub.h"

namespace dcpp {

const char LuaManager::className[] = "DC";
Lunar<LuaManager>::RegType LuaManager::methods[] = {
    {"SendHubMessage", &LuaManager::SendHubMessage },
    {"SendClientMessage", &LuaManager::SendClientMessage },
    {"SendUDP", &LuaManager::SendUDPPacket},
    {"PrintDebug", &LuaManager::GenerateDebugMessage},
    {"GetClientIp", &LuaManager::GetClientIp},
    {"GetHubIpPort", &LuaManager::GetHubIpPort},
    {"GetHubUrl", &LuaManager::GetHubUrl},
    {"InjectHubMessage", &LuaManager::InjectHubMessageNMDC},
    {"InjectHubMessageADC", &LuaManager::InjectHubMessageADC},
    {"CreateClient", &LuaManager::CreateClient},
    {"DeleteClient", &LuaManager::DeleteClient},
    {"RunTimer", &LuaManager::RunTimer},
    {"GetSetting", &LuaManager::GetSetting},
    {"ToUtf8", &LuaManager::ToUtf8},
    {"FromUtf8", &LuaManager::FromUtf8},
    {"GetAppPath", &LuaManager::GetAppPath},
    {"GetConfigPath", &LuaManager::GetConfigPath},
    {"GetScriptsPath", &LuaManager::GetScriptsPath},
    {"GetConfigScriptsPath", &LuaManager::GetConfigScriptsPath},
    {"DropUserConnection", &LuaManager::DropUserConnection},
    {0, nullptr}
};

int LuaManager::DeleteClient(lua_State* L){
    if (lua_gettop(L) == 1 && lua_islightuserdata(L, -1)){
        Client* client = (Client*) lua_touserdata(L, -1);
        ClientManager::getInstance()->putClient(client);
    }
    return 0;
}

int LuaManager::CreateClient(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_isstring(L, -2) && lua_isstring(L, -1)){
        Client* client = ClientManager::getInstance()->getClient(lua_tostring(L, -2));
        Identity ident;
        ident.setNick(lua_tostring(L, -1));
        client->setMyIdentity(ident);
        client->setPassword("");
        client->connect();

        lua_pushlightuserdata(L, client);
        return 1;
    }

    return 0;
}

int LuaManager::InjectHubMessageNMDC(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_islightuserdata(L, -2) && lua_isstring(L, -1))
        reinterpret_cast<NmdcHub *>(lua_touserdata(L, -2))->onLine(lua_tostring(L, -1));

    return 0;
}

int LuaManager::InjectHubMessageADC(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_islightuserdata(L, -2) && lua_isstring(L, -1))
        reinterpret_cast<AdcHub *>(lua_touserdata(L, -2))->dispatch(lua_tostring(L, -1));

    return 0;
}

int LuaManager::SendClientMessage(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_islightuserdata(L, -2) && lua_isstring(L, -1)) {
        reinterpret_cast<UserConnection *>(lua_touserdata(L, -2))->sendRaw(lua_tostring(L, -1));
    }

    return 0;
}

int LuaManager::SendHubMessage(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_islightuserdata(L, -2) && lua_isstring(L, -1)) {
        reinterpret_cast<Client*>(lua_touserdata(L, -2))->send(lua_tostring(L, -1));
    }

    return 0;
}

int LuaManager::GenerateDebugMessage(lua_State* L) {
    if (lua_gettop(L) == 1 && lua_isstring(L, -1))
        ScriptManager::getInstance()->SendDebugMessage(lua_tostring(L, -1));

    return 0;
}

int LuaManager::SendUDPPacket(lua_State* L) {
    if (lua_gettop(L) == 2 && lua_isstring(L, -2) && lua_isstring(L, -1)) {
        StringList sl = StringTokenizer<string>(lua_tostring(L, -2), ':').getTokens();
        ScriptManager::getInstance()->s.writeTo(sl[0], sl[1], lua_tostring(L, -1), string(lua_tostring(L, -1)).size());
    }

    return 0;
}

int LuaManager::DropUserConnection(lua_State* L) {
    if (lua_gettop(L) == 1 && lua_islightuserdata(L, -1)) {
        reinterpret_cast<UserConnection *>(lua_touserdata(L, -1))->disconnect();
    }

    return 0;
}

} // namespace dcpp
