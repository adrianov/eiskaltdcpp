/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "UserConnection.h"

#include "AdcCommand.h"
#include "DebugManager.h"
#include "format.h"
#include "StringTokenizer.h"
#ifdef LUA_SCRIPT
#include "ScriptManager.h"
#endif

namespace dcpp {

void UserConnection::on(BufferedSocketListener::Line, const string& aLine) noexcept {
    if(aLine.length() < 2) {
        fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
        return;
    }

    if(aLine[0] == 'C' && !isSet(FLAG_NMDC)) {
        if(!Text::validateUtf8(aLine)) {
            fire(UserConnectionListener::ProtocolError(), this, _("Non-UTF-8 data in an ADC connection"));
            return;
        }
        COMMAND_DEBUG(aLine, DebugManager::CLIENT_IN, getRemoteIp());
        dispatch(aLine);
        return;
    } else if(aLine[0] == '$') {
        setFlag(FLAG_NMDC);
    } else {
        fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
        return;
    }
    COMMAND_DEBUG((Util::stricmp(getEncoding(), Text::utf8) != 0 ? Text::toUtf8(aLine, getEncoding()) : aLine), DebugManager::CLIENT_IN, getRemoteIp());
    string cmd;
    string param;

    string::size_type x;
#ifdef LUA_SCRIPT
    if(onUserConnectionMessageIn(this, aLine)) {
        disconnect(true);
        return;
    }
#endif

    if( (x = aLine.find(' ')) == string::npos) {
        cmd = aLine;
    } else {
        cmd = aLine.substr(0, x);
        param = aLine.substr(x+1);
    }

    if(cmd == "$MyNick") {
        if(!param.empty())
            fire(UserConnectionListener::MyNick(), this, param);
    } else if(cmd == "$Direction") {
        x = param.find(" ");
        if(x != string::npos) {
            fire(UserConnectionListener::Direction(), this, param.substr(0, x), param.substr(x+1));
        }
    } else if(cmd == "$Error") {
        static const char goneSuffix[] = " no more exists";
        const size_t goneLen = sizeof(goneSuffix) - 1;
        if(Util::stricmp(param.c_str(), FILE_NOT_AVAILABLE) == 0 ||
                (param.length() >= goneLen && param.compare(param.length() - goneLen, goneLen, goneSuffix) == 0)) {
            fire(UserConnectionListener::FileNotAvailable(), this);
        } else {
            fire(UserConnectionListener::ProtocolError(), this, param);
        }
    } else if(cmd == "$GetListLen") {
        fire(UserConnectionListener::GetListLength(), this);
    } else if(cmd == "$Get") {
        x = param.find('$');
        if(x != string::npos) {
            fire(UserConnectionListener::Get(), this, Text::toUtf8(param.substr(0, x), encoding), Util::toInt64(param.substr(x+1)) - (int64_t)1);
        }
    } else if(cmd == "$Key") {
        if(!param.empty())
            fire(UserConnectionListener::Key(), this, param);
    } else if(cmd == "$Lock") {
        if(!param.empty()) {
            x = param.find(" Pk=");
            if(x != string::npos) {
                fire(UserConnectionListener::CLock(), this, param.substr(0, x), param.substr(x + 4));
            } else {
                x = param.find(' ');
                if(x != string::npos) {
                    setFlag(FLAG_INVALIDKEY);
                    fire(UserConnectionListener::CLock(), this, param.substr(0, x), Util::emptyString);
                } else {
                    fire(UserConnectionListener::CLock(), this, param, Util::emptyString);
                }
            }
        }
    } else if(cmd == "$Send") {
        fire(UserConnectionListener::Send(), this);
    } else if(cmd == "$MaxedOut") {
        size_t queuePos = 0;
        if(!param.empty()) {
            string pos = param;
            if(!pos.empty() && pos.back() == '|')
                pos.pop_back();
            if(!pos.empty())
                queuePos = Util::toUInt(pos);
        }
        fire(UserConnectionListener::MaxedOut(), this, queuePos);
    } else if(cmd == "$Supports") {
        if(!param.empty()) {
            fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(param, ' ').getTokens());
        }
    } else if(cmd.compare(0, 4, "$ADC") == 0) {
        dispatch(aLine, true);
    } else {
        fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
    }
}

} // namespace dcpp
