/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <cstdint>
#include <cmath>

namespace TransfersDisplay {

constexpr double SpeedStep = 100.0 * 1024.0;
constexpr int64_t TimeLeftStep = 30;

inline int64_t roundSpeed(double speed)
{
    if (speed <= 0.0)
        return static_cast<int64_t>(speed);
    return static_cast<int64_t>(std::round(speed / SpeedStep) * SpeedStep);
}

inline int64_t smoothTimeLeft(int64_t displayed, int64_t actual)
{
    if (actual < 0)
        return actual;
    if (displayed < 0)
        return actual;
    if (actual <= displayed)
        return actual;
    if (actual > displayed + TimeLeftStep)
        return displayed + TimeLeftStep;
    return displayed;
}

} // namespace TransfersDisplay
