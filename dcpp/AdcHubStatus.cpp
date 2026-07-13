/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC quit, user-command, and status handlers.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ChatMessage.h"
#include "ConnectionManager.h"
#include "format.h"
#include "HubSearchDenied.h"
#include "UserCommand.h"

namespace dcpp {

void AdcHub::handle(AdcCommand::QUI, AdcCommand& c) noexcept {
    uint32_t s = AdcCommand::toSID(c.getParam(0));

    OnlineUser* victim = findUser(s);
    if(victim) {

        string tmp;
        if(c.getParam("MS", 1, tmp)) {
            OnlineUser* source = 0;
            string tmp2;
            if(c.getParam("ID", 1, tmp2)) {
                source = findUser(AdcCommand::toSID(tmp2));
            }

            if(source) {
                tmp = str(F_("%1% was kicked by %2%: %3%") % victim->getIdentity().getNick() %
                          source->getIdentity().getNick() % tmp);
            } else {
                tmp = str(F_("%1% was kicked: %2%") % victim->getIdentity().getNick() % tmp);
            }
            fire(ClientListener::StatusMessage(), this, tmp, ClientListener::FLAG_IS_SPAM);
        }

        putUser(s, c.getParam("DI", 1, tmp));
    }

    if(s == sid) {
        // this QUI is directed to us

        string tmp;
        if(c.getParam("TL", 1, tmp)) {
            if(tmp == "-1") {
                setAutoReconnect(false);
            } else {
                setAutoReconnect(true);
                setReconnAttempts(0);
                setReconnDelay(Util::toUInt32(tmp));
            }
        }
        if(!victim && c.getParam("MS", 1, tmp)) {
            fire(ClientListener::StatusMessage(), this, tmp, ClientListener::FLAG_NORMAL);
        }
        if(c.getParam("RD", 1, tmp)) {
            if(!handleRedirect(tmp))
                fire(ClientListener::Redirect(), this, tmp);
        }
    }
}

void AdcHub::handle(AdcCommand::CMD, AdcCommand& c) noexcept {
    if(c.getParameters().empty())
        return;
    const string& name = c.getParam(0);
    bool rem = c.hasFlag("RM", 1);
    if(rem) {
        fire(ClientListener::HubUserCommand(), this, (int)UserCommand::TYPE_REMOVE, 0, name, Util::emptyString);
        return;
    }
    bool sep = c.hasFlag("SP", 1);
    string sctx;
    if(!c.getParam("CT", 1, sctx))
        return;
    int ctx = Util::toInt(sctx);
    if(ctx <= 0)
        return;
    if(sep) {
        fire(ClientListener::HubUserCommand(), this, (int)UserCommand::TYPE_SEPARATOR, ctx, name, Util::emptyString);
        return;
    }
    bool once = c.hasFlag("CO", 1);
    string txt;
    if(!c.getParam("TT", 1, txt))
        return;
    fire(ClientListener::HubUserCommand(), this, (int)(once ? UserCommand::TYPE_RAW_ONCE : UserCommand::TYPE_RAW), ctx, name, txt);
}

void AdcHub::handle(AdcCommand::STA, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 2)
        return;

    OnlineUser* u = c.getFrom() == AdcCommand::HUB_SID ? &getUser(c.getFrom(), CID()) : findUser(c.getFrom());
    if(!u)
        return;

    //int severity = Util::toInt(c.getParam(0).substr(0, 1));
    if(c.getParam(0).size() != 3) {
        return;
    }

    switch(Util::toInt(c.getParam(0).substr(1))) {

    case AdcCommand::ERROR_BAD_PASSWORD:
    {
        if(c.getType() == AdcCommand::TYPE_INFO) {
            setPassword(Util::emptyString);
        }
        break;
    }

    case AdcCommand::ERROR_COMMAND_ACCESS:
    {
        if(c.getType() == AdcCommand::TYPE_INFO) {
            string tmp;
            if(c.getParam("FC", 1, tmp) && tmp.size() == 4)
                forbiddenCommands.insert(AdcCommand::toFourCC(tmp.c_str()));
        }
        break;
    }

    case AdcCommand::ERROR_PROTOCOL_UNSUPPORTED:
    {
        string tmp;
        if(c.getParam("PR", 1, tmp)) {
            if(tmp == CLIENT_PROTOCOL) {
                u->getUser()->setFlag(User::NO_ADC_1_0_PROTOCOL);
            } else if(tmp == SECURE_CLIENT_PROTOCOL_TEST) {
                u->getUser()->setFlag(User::NO_ADCS_0_10_PROTOCOL);
                u->getUser()->unsetFlag(User::TLS);
            }
            // Try again...
            ConnectionManager::getInstance()->force(u->getUser());
        }
        return;
    }

    case AdcCommand::ERROR_NICK_TAKEN:
        if(u->getUser() == getMyIdentity().getUser() && tryAlternateNick())
            return;
        disconnect(false);
        fire(ClientListener::NickTaken(), this);
        return;
    }
    noteSearchDenied(*this, c.getParam(1));
    noteSearchRateLimit(searchQueue, c.getParam(1));
    ChatMessage message = { c.getParam(1), u, nullptr, nullptr, false, 0 };
    fire(ClientListener::Message(), this, message);
}

} // namespace dcpp
