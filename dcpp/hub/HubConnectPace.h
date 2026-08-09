/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <atomic>
#include <cstdint>

#include "../forward.h"
#include "../TimerManager.h"

namespace dcpp {

/**
 * Per-hub peer-connect wait (Ledokol ctmuptime, PtokaX/uHub CTM flood).
 * Parses hub text, holds the pause, learns min online time, and reapplies it
 * the next time the hub becomes ready.
 */
class HubConnectPace
{
public:
    HubConnectPace() : nextAllowed(0), readyAt(0), floorMs(0) {}

    /** True when an outgoing CTM/RCM on this hub may be sent. */
    bool allow() const {
        return GET_TICK() >= nextAllowed.load(std::memory_order_relaxed);
    }

    /** Parse hub chat/status text and pause when it asks us to wait. */
    void note(const string& hubUrl, const string& message);

    /** Hub reached NORMAL — apply any learned min online time for this URL. */
    void ready(const string& hubUrl);

private:
    void delay(uint32_t seconds);
    void delayUntil(uint64_t tick);

    /** Earliest tick for the next CTM/RCM; 0 = no pause. */
    std::atomic<uint64_t> nextAllowed;
    /** Tick when the hub last became ready (STATE_NORMAL). */
    std::atomic<uint64_t> readyAt;
    /** Learned minimum online ms before CTM (Ledokol ctmuptime). */
    std::atomic<uint64_t> floorMs;
};

} // namespace dcpp
