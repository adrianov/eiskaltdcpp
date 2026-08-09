/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC chat, user commands, and password challenge/response.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ChatMessage.h"
#include "hub/HubSearchDenied.h"
#include "UserCommand.h"
#include "Util.h"

namespace dcpp {

void AdcHub::hubMessage(const string& aMessage, bool thirdPerson) {
    if(state != STATE_NORMAL)
        return;
    AdcCommand c(AdcCommand::CMD_MSG, AdcCommand::TYPE_BROADCAST);
    c.addParam(aMessage);
    if(thirdPerson)
        c.addParam("ME", "1");
    send(c);
}

void AdcHub::privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson) {
    if(state != STATE_NORMAL)
        return;
    AdcCommand c(AdcCommand::CMD_MSG, user.getIdentity().getSID(), AdcCommand::TYPE_ECHO);
    c.addParam(aMessage);
    if(thirdPerson)
        c.addParam("ME", "1");
    c.addParam("PM", getMySID());
    send(c);
}

void AdcHub::sendUserCmd(const UserCommand& command, const ParamMap& params) {
    if(state != STATE_NORMAL)
        return;
    string cmd = Util::formatParams(command.getCommand(), params, false);
    if(command.isChat()) {
        if(command.getTo().empty()) {
            hubMessage(cmd);
        } else {
            const string& to = command.getTo();
            Lock l(cs);
            for(auto& i: users) {
                if(i.second->getIdentity().getNick() == to) {
                    privateMessage(*i.second, cmd);
                    return;
                }
            }
        }
    } else {
        send(cmd);
    }
}

void AdcHub::password(const string& pwd) {
    if(state != STATE_VERIFY)
        return;
    if(!salt.empty()) {
        size_t saltBytes = salt.size() * 5 / 8;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[saltBytes]);
        Encoder::fromBase32(salt.c_str(), &buf[0], saltBytes);
        TigerHash th;
        if(oldPassword) {
            CID cid = getMyIdentity().getUser()->getCID();
            th.update(cid.data(), CID::SIZE);
        }
        th.update(pwd.data(), pwd.length());
        th.update(&buf[0], saltBytes);
        send(AdcCommand(AdcCommand::CMD_PAS, AdcCommand::TYPE_HUB).addParam(Encoder::toBase32(th.finalize(), TigerHash::BYTES)));
        salt.clear();
    }
}

void AdcHub::handle(AdcCommand::MSG, AdcCommand& c) noexcept {
    if(c.getParameters().empty())
        return;

    ChatMessage message = { c.getParam(0), findUser(c.getFrom()), nullptr, nullptr, false, 0 };

    if(!message.from)
        return;

    string temp;
    if(c.getParam("PM", 1, temp)) { // add PM<group-cid> as well
        message.to = findUser(c.getTo());
        if(!message.to)
            return;

        message.replyTo = findUser(AdcCommand::toSID(temp));
        if(!message.replyTo)
            return;
    }

    message.thirdPerson = c.hasFlag("ME", 1);

    if(c.getParam("TS", 1, temp))
        message.timestamp = Util::toInt64(temp);

    noteHubLimits(c.getParam(0));

    const Identity& id = message.from->getIdentity();
    if(id.isHub() || id.isBot() || id.isOp() || message.from->getUser()->isSet(User::BOT))
        noteSearchDenied(*this, c.getParam(0));

    fire(ClientListener::Message(), this, message);
}

void AdcHub::handle(AdcCommand::GPA, AdcCommand& c) noexcept {
    if(c.getParameters().empty())
        return;
    salt = c.getParam(0);
    state = STATE_VERIFY;

    fire(ClientListener::GetPassword(), this);
}

} // namespace dcpp
