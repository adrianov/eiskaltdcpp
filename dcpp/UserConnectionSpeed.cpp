/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UserConnection.h"

namespace dcpp {

static const uint64_t SPEED_WINDOW_MS = 5000;
static const uint64_t SPEED_STALE_MS = 2500;
static const double SPEED_EMA_BLEND = 0.25;

void UserConnection::resetTransferSpeed() noexcept {
    Lock l(speedCs);
    speedSamples.clear();
    displaySpeed = 0;
    lastSpeedTick = 0;
}

void UserConnection::updateTransferSpeed(int64_t cumulativeBytes) noexcept {
    const uint64_t tick = GET_TICK();

    Lock l(speedCs);

    if(!speedSamples.empty() && cumulativeBytes < speedSamples.back().second) {
        speedSamples.clear();
        displaySpeed = 0;
    }

    if(!speedSamples.empty() && speedSamples.back().second == cumulativeBytes) {
        speedSamples.back().first = tick;
        lastSpeedTick = tick;
        return;
    }

    speedSamples.emplace_back(tick, cumulativeBytes);

    while(speedSamples.size() > 2 && tick - speedSamples.front().first > SPEED_WINDOW_MS)
        speedSamples.pop_front();

    if(speedSamples.size() >= 2) {
        const uint64_t dt = speedSamples.back().first - speedSamples.front().first;
        const int64_t db = speedSamples.back().second - speedSamples.front().second;
        if(dt > 0 && db > 0) {
            const double instant = (static_cast<double>(db) / static_cast<double>(dt)) * 1000.0;
            if(displaySpeed <= 0)
                displaySpeed = instant;
            else
                displaySpeed = displaySpeed * (1.0 - SPEED_EMA_BLEND) + instant * SPEED_EMA_BLEND;
        }
    }

    lastSpeedTick = tick;
}

double UserConnection::getDisplaySpeed() const noexcept {
    Lock l(speedCs);

    if(displaySpeed <= 0 || lastSpeedTick == 0)
        return 0;

    const uint64_t idle = GET_TICK() - lastSpeedTick;
    if(idle <= SPEED_STALE_MS)
        return displaySpeed;

    if(idle >= SPEED_STALE_MS * 2)
        return 0;

    return displaySpeed * (1.0 - static_cast<double>(idle - SPEED_STALE_MS) / SPEED_STALE_MS);
}

} // namespace dcpp
