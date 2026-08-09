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

#include "forward.h"
#include "TimerManager.h"

namespace dcpp {

/**
 * Per-hub peer-connect pacing.
 * Hubs (Ledokol ctmuptime, PtokaX/uHub CTM flood) tell clients to wait before
 * more $ConnectToMe / CTM; this holds that pause and parses the hub text.
 */
class HubConnectPace
{
public:
    HubConnectPace() : nextAllowed(0) {}

    /** True when an outgoing CTM/RCM on this hub may be sent. */
    bool allow() const {
        return GET_TICK() >= nextAllowed.load(std::memory_order_relaxed);
    }

    /** Defer further peer connects for at least `seconds`. */
    void delay(uint32_t seconds);

    /** Parse hub chat/status text and delay when it asks for a connect wait. */
    void note(const string& message);

private:
    /** Earliest tick for the next CTM/RCM; 0 = no pause. */
    std::atomic<uint64_t> nextAllowed;
};

} // namespace dcpp
