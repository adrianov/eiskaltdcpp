/*
 * Copyright (C) 2010 cologic, ne5@parsoma.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LuaManager query / path helpers for ScriptManager.
 */

#include "stdinc.h"

#include "ScriptManager.h"
#include "Util.h"
#include "Client.h"
#include "UserConnection.h"
#include "SettingsManager.h"

namespace dcpp {

int LuaManager::GetSetting(lua_State* L) {
    int n;
    SettingsManager::Types type;
    if(lua_gettop(L) == 1 && lua_isstring(L, -1) && SettingsManager::getInstance()->getType(lua_tostring(L, -1), n, type)) {
        if(type == SettingsManager::TYPE_STRING) {
            lua_pushstring(L, SettingsManager::getInstance()->get((SettingsManager::StrSetting)n).c_str());
            return 1;
        } else if(type == SettingsManager::TYPE_INT) {
            lua_pushnumber(L, SettingsManager::getInstance()->get((SettingsManager::IntSetting)n));
            return 1;
        } else if(type == SettingsManager::TYPE_INT64) {
            lua_pushnumber(L, static_cast<lua_Number>(SettingsManager::getInstance()->get((SettingsManager::Int64Setting)n)));
            return 1;
        }
    }
    lua_pushliteral(L, "GetSetting: setting not found");
    lua_error(L);
    return 0;
}

int LuaManager::ToUtf8(lua_State* L) {
    if(lua_gettop(L) == 1 && lua_isstring(L, -1) ) {
        lua_pushstring(L, Text::acpToUtf8(lua_tostring(L, -1)).c_str());
        return 1;
    } else {
        lua_pushliteral(L, "ToUtf8: string needed as argument");
        lua_error(L);
    }
    return 0;
}

int LuaManager::FromUtf8(lua_State* L) {
    if(lua_gettop(L) == 1 && lua_isstring(L, -1) ) {
        lua_pushstring(L, Text::utf8ToAcp(lua_tostring(L, -1)).c_str());
        return 1;
    } else {
        lua_pushliteral(L, "FromUtf8: string needed as argument");
        lua_error(L);
    }
    return 0;
}

int LuaManager::GetAppPath(lua_State* L) {
    lua_pushstring(L, Text::utf8ToAcp(Util::getPath(Util::PATH_RESOURCES)).c_str());
    return 1;
}

int LuaManager::GetConfigPath(lua_State* L) {
    lua_pushstring(L, (Text::utf8ToAcp(Util::getPath(Util::PATH_USER_CONFIG)) + PATH_SEPARATOR).c_str());
    return 1;
}

int LuaManager::GetConfigScriptsPath(lua_State* L) {
    lua_pushstring(L, (Text::utf8ToAcp(Util::getPath(Util::PATH_USER_CONFIG)) + PATH_SEPARATOR).c_str());
    return 1;
}

int LuaManager::GetScriptsPath(lua_State* L) {
    string scripts_path;
    scripts_path = Text::utf8ToAcp(Util::getPath(Util::PATH_USER_CONFIG)) + "luascripts" + PATH_SEPARATOR;

    if(!Util::fileExists(scripts_path)) {
#ifdef _WIN32
        scripts_path = Text::utf8ToAcp(Util::getPath(Util::PATH_GLOBAL_CONFIG)) + "resources" + PATH_SEPARATOR + "luascripts" + PATH_SEPARATOR;
#else
        scripts_path = string(_DATADIR) + PATH_SEPARATOR + "luascripts" + PATH_SEPARATOR;
#endif
    }
    lua_pushstring(L, scripts_path.c_str());
    return 1;
}

int LuaManager::GetClientIp(lua_State* L) {
    UserConnection* uc = (UserConnection*)lua_touserdata(L, 1);
    if(uc == NULL) {
        lua_pushliteral(L, "GetClientIpPort: missing client pointer");
        lua_error(L);
        return 0;
    }
    lua_pushstring(L, uc->getRemoteIp().c_str());
    return 1;
}

int LuaManager::GetHubIpPort(lua_State* L) {
    Client* c = (Client*)lua_touserdata(L, 1);
    if(c == NULL) {
        lua_pushliteral(L, "GetHubIpPort: missing hub pointer");
        lua_error(L);
        return 0;
    }
    lua_pushstring(L, c->getIpPort().c_str());
    return 1;
}

int LuaManager::GetHubUrl(lua_State* L) {
    Client* c = (Client*)lua_touserdata(L, 1);
    if(c == NULL) {
        lua_pushliteral(L, "GetHubUrl: missing hub pointer");
        lua_error(L);
        return 0;
    }
    lua_pushstring(L, c->getHubUrl().c_str());
    return 1;
}

int LuaManager::RunTimer(lua_State* L) {
    if(lua_gettop(L) == 1 && lua_isnumber(L, -1)) {
        bool on = lua_tonumber(L, 1) != 0;
        ScriptManager* sm = ScriptManager::getInstance();
        if(on != sm->getTimerEnabled()) {
            if(on)
                TimerManager::getInstance()->addListener(sm);
            else
                TimerManager::getInstance()->removeListener(sm);
            sm->setTimerEnabled(on);
        }
    } else {
        lua_pushliteral(L, "RunTimer: missing integer (0=off,!0=on)");
        lua_error(L);
        return 0;
    }
    return 1;
}

} // namespace dcpp
