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

#include "CriticalSection.h"
#include "TimerManager.h"
#include "Util.h"

#include <unordered_map>

namespace dcpp {

namespace {

/** Pause when the hub reports CTM flood without a duration. */
constexpr uint32_t FLOOD_PAUSE_SEC = 6;

CriticalSection floorCs;
/** hubUrl → learned minimum online ms before peer connects. */
std::unordered_map<string, uint64_t> learnedFloor;

uint64_t loadFloor(const string& hubUrl) {
    if(hubUrl.empty())
        return 0;
    Lock l(floorCs);
    const auto i = learnedFloor.find(hubUrl);
    return i == learnedFloor.end() ? 0 : i->second;
}

void storeFloor(const string& hubUrl, uint64_t ms) {
    if(hubUrl.empty() || !ms)
        return;
    Lock l(floorCs);
    uint64_t& slot = learnedFloor[hubUrl];
    if(ms > slot)
        slot = ms;
}

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
 * Phrases stay specific so login/IP "connection" notices are ignored.
 */
bool isPeerConnectWait(const string& message) {
    static const char* const keys[] = {
        "connecting to other",
        "connecting to more",
        "connect to other users",
        "connect to more users",
        "подключаться к другим",
        "подключаться к большему",
        "соединяться с другими",
    };
    for(const char* key : keys) {
        if(Util::findSubString(message, key) != string::npos)
            return true;
    }
    return false;
}

bool isConnectFlood(const string& message) {
    const bool flooded = Util::findSubString(message, "flood") != string::npos
        || Util::findSubString(message, "флуд") != string::npos;
    if(!flooded)
        return false;
    return Util::findSubString(message, "ConnectToMe") != string::npos
        || Util::findSubString(message, "RevConnectToMe") != string::npos
        || Util::findSubString(message, "connect flood") != string::npos;
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

void HubConnectPace::delayUntil(uint64_t tick) {
    uint64_t cur = nextAllowed.load(std::memory_order_relaxed);
    while(cur < tick && !nextAllowed.compare_exchange_weak(
            cur, tick, std::memory_order_relaxed, std::memory_order_relaxed))
    { }
}

void HubConnectPace::delay(uint32_t seconds) {
    if(!seconds)
        return;
    delayUntil(GET_TICK() + static_cast<uint64_t>(seconds) * 1000);
}

void HubConnectPace::ready(const string& hubUrl) {
    const uint64_t now = GET_TICK();
    readyAt.store(now, std::memory_order_relaxed);
    const uint64_t floor = loadFloor(hubUrl);
    floorMs.store(floor, std::memory_order_relaxed);
    if(floor)
        delayUntil(now + floor);
}

void HubConnectPace::note(const string& hubUrl, const string& message) {
    const uint32_t seconds = parsePauseSeconds(message);
    if(!seconds)
        return;
    delay(seconds);

    // Remaining wait + time already online ≈ required min online time.
    const uint64_t ra = readyAt.load(std::memory_order_relaxed);
    if(!ra || !isPeerConnectWait(message))
        return;
    const uint64_t need = (GET_TICK() - ra) + static_cast<uint64_t>(seconds) * 1000;
    uint64_t cur = floorMs.load(std::memory_order_relaxed);
    if(need > cur) {
        floorMs.store(need, std::memory_order_relaxed);
        storeFloor(hubUrl, need);
    }
}

} // namespace dcpp
