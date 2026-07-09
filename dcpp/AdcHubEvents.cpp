/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC socket lifecycle and keep-alive timer handlers.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "SettingsManager.h"
#include "Text.h"

namespace dcpp {

void AdcHub::on(Connected c) noexcept {
    Client::on(c);

    if(state != STATE_PROTOCOL) {
        return;
    }

    lastInfoMap.clear();
    sid = 0;
    forbiddenCommands.clear();

    AdcCommand cmd(AdcCommand::CMD_SUP, AdcCommand::TYPE_HUB);
    cmd.addParam(BAS0_SUPPORT).addParam(BASE_SUPPORT).addParam(TIGR_SUPPORT);

    if(BOOLSETTING(HUB_USER_COMMANDS)) {
        cmd.addParam(UCM0_SUPPORT);
    }
    if(BOOLSETTING(SEND_BLOOM)) {
        cmd.addParam(BLO0_SUPPORT);
    }
    cmd.addParam(ZLIF_SUPPORT);
#ifdef WITH_DHT
    if (BOOLSETTING(USE_DHT))
        cmd.addParam(DHT0_SUPPORT);
#endif
    send(cmd);
}

void AdcHub::on(Line l, const string& aLine) noexcept {
    Client::on(l, aLine);

    if(!Text::validateUtf8(aLine)) {
        // @todo report to user?
        return;
    }

    if(BOOLSETTING(ADC_DEBUG)) {
        fire(ClientListener::StatusMessage(), this, "<ADC>" + aLine + "</ADC>");
    }
#ifdef LUA_SCRIPT
    if (onClientMessage(this, aLine))
        return;
#endif
    dispatch(aLine);
}

void AdcHub::on(Failed f, const string& aLine) noexcept {
    clearUsers();
    Client::on(f, aLine);
}

void AdcHub::on(Second s, uint64_t aTick) noexcept {
    Client::on(s, aTick);
    if(state == STATE_NORMAL && (aTick > (getLastActivity() + 120*1000)) ) {
        send("\n", 1);
    }
}

#ifdef LUA_SCRIPT
bool AdcScriptInstance::onClientMessage(AdcHub* aClient, const string& aLine) {
    Lock l(cs);
    MakeCall("adch", "DataArrival", 1, aClient, aLine);
    return GetLuaBool();

}
#endif

} // namespace dcpp
