/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC chat and password-challenge handlers.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ChatMessage.h"
#include "HubSearchDenied.h"

namespace dcpp {

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
