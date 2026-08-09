/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "hub/HubConnectPace.h"

#include "TimerManager.h"
#include "Util.h"

namespace dcpp {

namespace {

/** Pause when the hub reports CTM flood without a duration. */
constexpr uint32_t FLOOD_PAUSE_SEC = 6;

bool unitAt(const string& message, string::size_type i, const char* unit) {
    return Util::findSubString(message, unit, i) == i;
}

/** First "N sec(onds)" / "N секунд…" in the message; 0 if none or out of range. */
uint32_t parseSeconds(const string& message) {
    for(string::size_type i = 0; i < message.size(); ++i) {
        if(!isdigit(static_cast<unsigned char>(message[i])))
            continue;
        string::size_type j = i;
        while(j < message.size() && isdigit(static_cast<unsigned char>(message[j])))
            ++j;
        if(j == i || j - i > 4)
            continue;
        const int n = Util::toInt(message.substr(i, j - i));
        string::size_type u = j;
        while(u < message.size() && isspace(static_cast<unsigned char>(message[u])))
            ++u;
        if(u >= message.size())
            break;
        if(unitAt(message, u, "sec") || unitAt(message, u, "секунд")
                || unitAt(message, u, "секунда") || unitAt(message, u, "секунды")) {
            if(n < 1 || n > 3600)
                return 0;
            return static_cast<uint32_t>(n);
        }
        i = j;
    }
    return 0;
}

/**
 * Ledokol ctmuptime and similar: wait before further peer connects.
 * Keep phrasing tight so login/IP "connection" notices are ignored.
 */
bool isPeerConnectWait(const string& message) {
    static const char* const keys[] = {
        "connecting to other",
        "connecting to more",
        "before connecting",
        "connect to other users",
        "connect to more users",
        "подключаться к другим",
        "подключаться к большему",
        "прежде чем подключаться",
        "перед подключением к",
        "соединяться с другими",
        "прежде чем соединяться",
    };
    for(const char* key : keys) {
        if(Util::findSubString(message, key) != string::npos)
            return true;
    }
    return false;
}

bool isConnectFlood(const string& message) {
    if(Util::findSubString(message, "flood") == string::npos)
        return false;
    return Util::findSubString(message, "ConnectToMe") != string::npos
        || Util::findSubString(message, "RevConnectToMe") != string::npos
        || Util::findSubString(message, "connect flood") != string::npos
        || Util::findSubString(message, "флуд ConnectToMe") != string::npos
        || Util::findSubString(message, "флуд с ConnectToMe") != string::npos;
}

uint32_t parsePauseSeconds(const string& message) {
    if(message.empty())
        return 0;
    if(isPeerConnectWait(message)) {
        if(const uint32_t sec = parseSeconds(message))
            return sec;
    }
    if(isConnectFlood(message))
        return FLOOD_PAUSE_SEC;
    return 0;
}

} // namespace

void HubConnectPace::delay(uint32_t seconds) {
    if(!seconds)
        return;
    const uint64_t until = GET_TICK() + static_cast<uint64_t>(seconds) * 1000;
    uint64_t cur = nextAllowed.load(std::memory_order_relaxed);
    while(cur < until && !nextAllowed.compare_exchange_weak(
            cur, until, std::memory_order_relaxed, std::memory_order_relaxed))
    { }
}

void HubConnectPace::note(const string& message) {
    const uint32_t seconds = parsePauseSeconds(message);
    if(seconds)
        delay(seconds);
}

} // namespace dcpp
