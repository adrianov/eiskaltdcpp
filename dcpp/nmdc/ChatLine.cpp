/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "ChatLine.h"

#include "../ChatMessage.h"
#include "../ClientListener.h"
#include "../hub/HubSearchDenied.h"
#include "../NmdcHub.h"
#include "../User.h"

namespace dcpp {

bool NmdcChatLine::mentionsBan(const string& text) {
    return Util::findSubString(text, "banned") != string::npos
        || Util::findSubString(text, "забанен") != string::npos;
}

void NmdcChatLine::honorLimits(const string& text) {
    if(mentionsBan(text))
        hub.setAutoReconnect(false);
    hub.noteHubLimits(text);
}

void NmdcChatLine::status(const string& text, int flags) {
    hub.fire(ClientListener::StatusMessage(), &hub, text, flags);
}

void NmdcChatLine::chat(const string& nick, const string& message) {
    string body = NmdcHub::unescape(message);
    honorLimits(body);
    hub.stopInfectedConnect(body, nick);
    hub.noteSecureCtmRejected(body);

    ChatMessage chatMessage = { body, hub.findUser(nick), nullptr, nullptr, false, 0 };
    if(!chatMessage.from) {
        OnlineUser& o = hub.getUser(nick);
        o.getIdentity().setHub(true);
        o.getIdentity().setHidden(true);
        hub.updated(o);
        chatMessage.from = &o;
    }

    const Identity& id = chatMessage.from->getIdentity();
    if(id.isHub() || id.isBot() || id.isOp() || chatMessage.from->getUser()->isSet(User::BOT))
        noteSearchDenied(hub, body);

    hub.fire(ClientListener::Message(), &hub, chatMessage);
}

void NmdcChatLine::handle(const string& utf8Line) {
    if(utf8Line.empty())
        return;

    if(utf8Line[0] != '<') {
        const string text = NmdcHub::unescape(utf8Line);
        // Corrupted framing or embedded protocol (e.g. nulls + "$Search …") — drop.
        if(NmdcHub::hasControlChars(text) || text.find("$Search") != string::npos)
            return;
        honorLimits(text);
        hub.stopInfectedConnect(text);
        hub.noteSecureCtmRejected(text);
        noteSearchDenied(hub, text);
        status(text);
        return;
    }

    string::size_type i = utf8Line.find('>', 2);
    if(i == string::npos) {
        // Malformed "<…$Search…" with no nick close — was shown as status.
        if(NmdcHub::hasControlChars(utf8Line) || utf8Line.find('$') != string::npos)
            return;
        const string text = NmdcHub::unescape(utf8Line);
        honorLimits(text);
        status(text);
        return;
    }

    string nick = utf8Line.substr(1, i - 1);
    if((utf8Line.length() - 1) <= i) {
        const string text = NmdcHub::unescape(utf8Line);
        honorLimits(text);
        status(text);
        return;
    }

    string message = utf8Line.substr(i + 2);
    if(!NmdcHub::isNickLike(nick)) {
        // Bot nicks with spaces (# Guard) still carry limit text.
        const string body = NmdcHub::unescape(message);
        honorLimits(body);
        status(NmdcHub::unescape(utf8Line));
        return;
    }
    if((utf8Line.find("Hub-Security") != string::npos) && (utf8Line.find("was kicked by") != string::npos)) {
        status(NmdcHub::unescape(utf8Line), ClientListener::FLAG_IS_SPAM);
        return;
    }
    if((utf8Line.find("is kicking") != string::npos) && (utf8Line.find("because:") != string::npos)) {
        status(NmdcHub::unescape(utf8Line), ClientListener::FLAG_IS_SPAM);
        return;
    }

    chat(nick, message);
}

} // namespace dcpp
