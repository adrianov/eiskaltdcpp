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

#include "stdinc.h"

#include "UserConnection.h"

#include "AdcCommand.h"
#include "ChatMessage.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "DebugManager.h"
#include "format.h"
#include "SettingsManager.h"
#include "Transfer.h"
#ifdef LUA_SCRIPT
#include "ScriptManager.h"
#endif

namespace dcpp {

const string UserConnection::FEATURE_MINISLOTS = "MiniSlots";
const string UserConnection::FEATURE_XML_BZLIST = "XmlBZList";
const string UserConnection::FEATURE_ADCGET = "ADCGet";
const string UserConnection::FEATURE_ZLIB_GET = "ZLIG";
const string UserConnection::FEATURE_TTHL = "TTHL";
const string UserConnection::FEATURE_TTHF = "TTHF";
const string UserConnection::FEATURE_ADC_BAS0 = "BAS0";
const string UserConnection::FEATURE_ADC_BASE = "BASE";
const string UserConnection::FEATURE_ADC_BZIP = "BZIP";
const string UserConnection::FEATURE_ADC_TIGR = "TIGR";

const string UserConnection::FILE_NOT_AVAILABLE = "File Not Available";

const string UserConnection::UPLOAD = "Upload";
const string UserConnection::DOWNLOAD = "Download";

#ifdef LUA_SCRIPT
bool UserConnectionScriptInstance::onUserConnectionMessageIn(UserConnection* aConn, const string& aLine) {
    Lock l(cs);
    MakeCall("dcpp", "UserDataIn", 1, aConn, aLine);
    return GetLuaBool();
}

bool UserConnectionScriptInstance::onUserConnectionMessageOut(UserConnection* aConn, const string& aLine) {
    Lock l(cs);
    MakeCall("dcpp", "UserDataOut", 1, aConn, aLine);
    return GetLuaBool();
}
#endif

void UserConnection::connect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, UserPtr) {
    dcassert(!socket);

    port = aPort;
    socket = BufferedSocket::getSocket(0);
    socket->addListener(this);
    socket->connect(aServer, aPort, localPort, natRole, isSet(FLAG_SECURE), BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS), true, Socket::PROTO_DEFAULT);
}

void UserConnection::accept(const Socket& aServer) {
    dcassert(!socket);
    socket = BufferedSocket::getSocket(0);
    socket->addListener(this);
    socket->accept(aServer, isSet(FLAG_SECURE), BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS));
}

void UserConnection::inf(bool withToken) {
    AdcCommand c(AdcCommand::CMD_INF);
    c.addParam("ID", ClientManager::getInstance()->getMyCID().toBase32());
    if(withToken) {
        c.addParam("TO", getToken());
    }
    send(c);
}

void UserConnection::get(const string &aType, const string &aName, const int64_t aStart, const int64_t aBytes) {
    send(AdcCommand(AdcCommand::CMD_GET).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes)));
}

void UserConnection::snd(const string &aType, const string &aName, const int64_t aStart, const int64_t aBytes) {
    send(AdcCommand(AdcCommand::CMD_SND).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes)));
}

void UserConnection::sup(const StringList& features) {
    AdcCommand c(AdcCommand::CMD_SUP);
    for(auto& i: features)
        c.addParam(i);
    send(c);
}

void UserConnection::supports(const StringList& feat) {
    string x;
    for(auto& i: feat) {
        x+= i + ' ';
    }
    send("$Supports " + x + '|');
}

void UserConnection::handle(AdcCommand::STA t, const AdcCommand& c) {
    if(c.getParameters().size() >= 2) {
        const string& code = c.getParam(0);
        if(!code.empty() && code[0] - '0' == AdcCommand::SEV_FATAL) {
            fire(UserConnectionListener::ProtocolError(), this, c.getParam(1));
            return;
        }
    }

    fire(t, this, c);
}

void UserConnection::on(Connected) noexcept {
    lastActivity = GET_TICK();
    fire(UserConnectionListener::Connected(), this);
}

void UserConnection::on(Data, uint8_t* data, size_t len) noexcept {
    lastActivity = GET_TICK();
    fire(UserConnectionListener::Data(), this, data, len);
}

void UserConnection::on(BytesSent, size_t bytes, size_t actual) noexcept {
    lastActivity = GET_TICK();
    fire(UserConnectionListener::BytesSent(), this, bytes, actual);
}

void UserConnection::on(ModeChange) noexcept {
    lastActivity = GET_TICK();
    fire(UserConnectionListener::ModeChange(), this);
}

void UserConnection::on(TransmitDone) noexcept {
    fire(UserConnectionListener::TransmitDone(), this);
}

void UserConnection::on(Updated) noexcept {
    fire(UserConnectionListener::Updated(), this);
}

void UserConnection::on(Failed, const string& aLine) noexcept {
    fire(UserConnectionListener::Failed(), this, aLine);
    setState(STATE_UNCONNECTED);
    delete this;
}

// # ms we should aim for per segment
void UserConnection::send(const string &aString) {
    lastActivity = GET_TICK();
    COMMAND_DEBUG((Util::stricmp(getEncoding(), Text::utf8) != 0 ? Text::toUtf8(aString, getEncoding()) : aString), DebugManager::CLIENT_OUT, getRemoteIp());
#ifdef LUA_SCRIPT
    if(onUserConnectionMessageOut(this, aString)) {
        disconnect(true);
        return;
    }
#endif
    socket->write(aString);
}

} // namespace dcpp
