/*
 * Copyright (C) 2010 cologic, ne5@parsoma.net
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

#include "ScriptManager.h"
#include "Util.h"
#include "Client.h"
#include "ClientManager.h"
#include "AdcHub.h"
#include "SettingsManager.h"
#include <cstddef>

namespace {

void callalert(lua_State *L, int status) {
    if (status != 0) {
        lua_getglobal(L, "_ALERT");
        if (lua_isfunction(L, -1)) {
            lua_insert(L, -2);
            lua_call(L, 1, 0);
        } else {
            dcpp::ScriptManager::getInstance()->SendDebugMessage(
                dcpp::Text::acpToUtf8(dcpp::string("LUA ERROR: ") + lua_tostring(L, -2)));
            lua_pop(L, 2);
        }
    }
}

int aux_do(lua_State *L, int status) {
    if (status == 0)
        status = lua_pcall(L, 0, LUA_MULTRET, 0);
    callalert(L, status);
    return status;
}

} // namespace

LUALIB_API int lua_dofile(lua_State *L, const char *filename) {
    return aux_do(L, luaL_loadfile(L, filename));
}

LUALIB_API int lua_dobuffer(lua_State *L, const char *buff, size_t size, const char *name) {
    return aux_do(L, luaL_loadbuffer(L, buff, size, name));
}

LUALIB_API int lua_dostring(lua_State *L, const char *str) {
    return lua_dobuffer(L, str, strlen(str), str);
}

namespace dcpp {

lua_State* ScriptInstance::L = 0;
CriticalSection ScriptInstance::cs;

ScriptManager::ScriptManager() : timerEnabled(false) {
}

ScriptManager::~ScriptManager() {
    if (ClientManager::getInstance())
        ClientManager::getInstance()->removeListener(this);
    if (timerEnabled && TimerManager::getInstance())
        TimerManager::getInstance()->removeListener(this);

    Lock l(cs);
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

void ScriptManager::load() {
    L = luaL_newstate();
    luaL_openlibs(L);

    Lunar<LuaManager>::Register(L);

    uint32_t color = SETTING(TEXT_COLOR);
    string function =
            "dcpp = {_init_me_anyway = true}\n"
            "function dcpp.FormatChatText(hub, text)\n"
            "   text = string.gsub(text, \"([{}\\\\])\", \"\\%1\")\n"
            "   text = string.gsub(text, \"\\n\", \"\\\\line\\n\")\n"
            "text = string.gsub( text, \".\", function( c )\n"
            "   if string.byte( c ) >= 128 then\n"
            "       return string.format( \"\\\\'%02x\", string.byte( c ) )\n"
            "   else\n"
            "       return c\n"
            "   end\n"
            "end )"
            "   return \"{\\\\urtf1\\n\"..\n"
            "           \"{\\\\colortbl ;"
            "\\\\red" + Util::toString(color & 0xFF) +
            "\\\\green" + Util::toString((color >> 8) & 0xFF) +
            "\\\\blue" + Util::toString((color >> 16) & 0xFF) +
            ";}\\n\"..\n"
            "           \"\\\\cf1 \"..text..\"}\\n\"\n"
            "end\n";

    lua_dostring(L, function.c_str());
    lua_pop(L, lua_gettop(L));

    s.create(Socket::TYPE_UDP);
    ClientManager::getInstance()->addListener(this);
}

void ScriptManager::SendDebugMessage(const string &mess) {
    dcdebug("%s\n", mess.c_str());
}

bool ScriptInstance::GetLuaBool() {
    bool ret = false;
    if (L && lua_gettop(L) > 0) {
        ret = !lua_isnil(L, -1);
        lua_pop(L, 1);
    }
    return ret;
}

string ScriptInstance::GetClientType(Client* aClient) {
    return dynamic_cast<AdcHub *>(aClient)?"adch":"nmdch";
}

void ScriptManager::on(ClientDisconnected, Client* aClient) noexcept {
    MakeCall(GetClientType(aClient), "OnHubRemoved", 0, aClient);
}

void ScriptManager::on(ClientConnected, Client* aClient) noexcept {
    MakeCall(GetClientType(aClient), "OnHubAdded", 0, aClient);
}

void ScriptManager::on(Second, uint64_t /* ticks */) noexcept {
    MakeCall("dcpp", "OnTimer", 0, 0);
}

void ScriptInstance::LuaPush(int i) {
    if (L)
        lua_pushnumber(L, i);
}

void ScriptInstance::LuaPush(const string& s) {
    if (L)
        lua_pushlstring(L, s.data(), s.size());
}

bool ScriptInstance::MakeCallRaw(const string& table, const string& method, int args, int ret) noexcept {
    if (!L)
        return false;
    lua_getglobal(L, table.c_str());
    lua_pushstring(L, method.c_str());
    if (lua_istable(L, -2)) {
        lua_gettable(L, -2);
        lua_remove(L, -2);
        lua_insert(L, 1);
        if(lua_pcall(L, args, ret, 0) == 0) {
            dcassert(lua_gettop(L) == ret);
            return true;
        }
        const char *msg = lua_tostring(L, -1);
        string formatted_msg = (msg != NULL)?string("LUA Error: ") + msg:string("LUA Error: (unknown)");
        ScriptManager::getInstance()->SendDebugMessage(formatted_msg);
        dcassert(lua_gettop(L) == 1);
        lua_pop(L, 1);
    } else {
        lua_settop(L, 0);
    }
    return false;
}

} // namespace dcpp
