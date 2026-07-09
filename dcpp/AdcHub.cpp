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
#include "AdcHub.h"

#include "AdcCommand.h"
#include "BufferedSocket.h"

namespace dcpp {

using std::make_pair;

const string AdcHub::CLIENT_PROTOCOL("ADC/1.0");
const string AdcHub::SECURE_CLIENT_PROTOCOL_TEST("ADCS/0.10");
const string AdcHub::ADCS_FEATURE("ADC0");
const string AdcHub::TCP4_FEATURE("TCP4");
const string AdcHub::UDP4_FEATURE("UDP4");
const string AdcHub::NAT0_FEATURE("NAT0");
const string AdcHub::SEGA_FEATURE("SEGA");
const string AdcHub::BASE_SUPPORT("ADBASE");
const string AdcHub::BAS0_SUPPORT("ADBAS0");
const string AdcHub::TIGR_SUPPORT("ADTIGR");
const string AdcHub::UCM0_SUPPORT("ADUCM0");
const string AdcHub::BLO0_SUPPORT("ADBLO0");
const string AdcHub::ZLIF_SUPPORT("ADZLIF");
#ifdef WITH_DHT
const string AdcHub::DHT0_SUPPORT("ADDHT0");
#endif

const vector<StringList> AdcHub::searchExts;

AdcHub::AdcHub(const string& aHubURL, bool secure) :
    Client(aHubURL, '\n', secure, Socket::PROTO_ADC), oldPassword(false), sid(0) {
    TimerManager::getInstance()->addListener(this);
}

AdcHub::~AdcHub() {
    TimerManager::getInstance()->removeListener(this);
    clearUsers();
}

void AdcHub::sendUDP(const AdcCommand& cmd) noexcept {
    string command;
    string ip;
    string port;
    {
        Lock l(cs);
        auto i = users.find(cmd.getTo());
        if(i == users.end()) {
            dcdebug("AdcHub::sendUDP: invalid user\n");
            return;
        }
        OnlineUser& ou = *i->second;
        if(!ou.getIdentity().isUdpActive()) {
            return;
        }
        ip = ou.getIdentity().getIp();
        port = ou.getIdentity().getUdpPort();
        command = cmd.toString(ou.getUser()->getCID());
    }
    try {
        udp.writeTo(ip, port, command);
    } catch(const SocketException& e) {
        dcdebug("AdcHub::sendUDP: write failed: %s\n", e.getError().c_str());
        udp.close();
    }
}

void AdcHub::handle(AdcCommand::ZON, AdcCommand& c) noexcept {
    if(c.getType() == AdcCommand::TYPE_INFO) {
        try {
            sock->setMode(BufferedSocket::MODE_ZPIPE);
        } catch (const Exception& e) {
            dcdebug("AdcHub::handleZON failed with error: %s\n", e.getError().c_str());
        }
    }
}

void AdcHub::handle(AdcCommand::ZOF, AdcCommand& c) noexcept {
    if(c.getType() == AdcCommand::TYPE_INFO) {
        try {
            sock->setMode(BufferedSocket::MODE_LINE);
        } catch (const Exception& e) {
            dcdebug("AdcHub::handleZOF failed with error: %s\n", e.getError().c_str());
        }
    }
}

int64_t AdcHub::getAvailable() const {
    Lock l(cs);
    int64_t x = 0;
    for(auto& i: users) {
        x+=i.second->getIdentity().getBytesShared();
    }
    return x;
}

string AdcHub::checkNick(const string& aNick) {
    string tmp = aNick;
    for(size_t i = 0; i < aNick.size(); ++i) {
        if(static_cast<uint8_t>(tmp[i]) <= 32) {
            tmp[i] = '_';
        }
    }
    return tmp;
}

void AdcHub::send(const AdcCommand& cmd) {
    if(forbiddenCommands.find(AdcCommand::toFourCC(cmd.getFourCC().c_str())) == forbiddenCommands.end()) {
        if(cmd.getType() == AdcCommand::TYPE_UDP)
            sendUDP(cmd);
        send(cmd.toString(sid));
    }
}

void AdcHub::unknownProtocol(uint32_t target, const string& protocol, const string& token) {
    AdcCommand cmd(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown", AdcCommand::TYPE_DIRECT);
    cmd.setTo(target);
    cmd.addParam("PR", protocol);
    cmd.addParam("TO", token);

    send(cmd);
}

} // namespace dcpp
