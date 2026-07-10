/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC INF identity updates and SUP/SID protocol setup.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "format.h"

namespace dcpp {

void AdcHub::handle(AdcCommand::INF, AdcCommand& c) noexcept {
    if(c.getParameters().empty())
        return;

    string cid;

    OnlineUser* u = 0;
    if(c.getParam("ID", 0, cid)) {
        u = findUser(CID(cid));
        if(u) {
            if(u->getIdentity().getSID() != c.getFrom()) {
                // Same CID but different SID not allowed - buggy hub?
                string nick;
                if(!c.getParam("NI", 0, nick)) {
                    nick = "[nick unknown]";
                }
                fire(ClientListener::StatusMessage(), this, str(F_("%1% (%2%) has same CID {%3%} as %4% (%5%), ignoring")
                                                                % u->getIdentity().getNick() % u->getIdentity().getSIDString() % cid % nick % AdcCommand::fromSID(c.getFrom())),
                     ClientListener::FLAG_IS_SPAM);
                return;
            }
        } else {
            u = &getUser(c.getFrom(), CID(cid));
        }
    } else if(c.getFrom() == AdcCommand::HUB_SID) {
        u = &getUser(c.getFrom(), CID());
    } else {
        u = findUser(c.getFrom());
    }

    if(!u) {
        dcdebug("AdcHub::INF Unknown user / no ID\n");
        return;
    }

    for(auto& i: c.getParameters()) {
        if(i.length() < 2)
            continue;

        u->getIdentity().set(i.c_str(), i.substr(2));
    }

    if(u->getIdentity().isBot()) {
        u->getUser()->setFlag(User::BOT);
    } else {
        u->getUser()->unsetFlag(User::BOT);
    }

    if(u->getIdentity().supports(ADCS_FEATURE)) {
        u->getUser()->setFlag(User::TLS);
    }

    if(!u->getIdentity().get("US").empty()) {
        u->getIdentity().setConnection(str(F_("%1%/s") % Util::formatBytes(u->getIdentity().get("US"))));
    }

    if(u->getUser() == getMyIdentity().getUser()) {
        state = STATE_NORMAL;
        setAutoReconnect(true);
        setMyIdentity(u->getIdentity());
        storeHubNick();
        updateCounts(false);
    }

    if(u->getIdentity().isHub()) {
        setHubIdentity(u->getIdentity());
        fire(ClientListener::HubUpdated(), this);
    } else {
        updated(*u);
    }
}

void AdcHub::handle(AdcCommand::SUP, AdcCommand& c) noexcept {
    if(state != STATE_PROTOCOL) /** @todo SUP changes */
        return;
    bool baseOk = false;
    bool tigrOk = false;
    for(auto& i: c.getParameters()) {
        if(i == BAS0_SUPPORT) {
            baseOk = true;
            tigrOk = true;
        } else if(i == BASE_SUPPORT) {
            baseOk = true;
        } else if(i == TIGR_SUPPORT) {
            tigrOk = true;
        }
    }

    if(!baseOk) {
        fire(ClientListener::StatusMessage(), this, _("Failed to negotiate base protocol"));
        disconnect(false);
        return;
    } else if(!tigrOk) {
        oldPassword = true;
        // Some hubs fake BASE support without TIGR support =/
        fire(ClientListener::StatusMessage(), this, _("Hub probably uses an old version of ADC, please encourage the owner to upgrade"));
    }
}

void AdcHub::handle(AdcCommand::SID, AdcCommand& c) noexcept {
    if(state != STATE_PROTOCOL) {
        dcdebug("Invalid state for SID\n");
        return;
    }

    if(c.getParameters().empty())
        return;

    sid = AdcCommand::toSID(c.getParam(0));

    state = STATE_IDENTIFY;
    info(true);
}

} // namespace dcpp
