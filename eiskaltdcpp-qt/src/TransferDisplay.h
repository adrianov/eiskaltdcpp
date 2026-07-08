/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <cstdint>
#include <cmath>

namespace TransferDisplay {

constexpr double SpeedStep = 100.0 * 1024.0;
constexpr int64_t TimeLeftStep = 30;

inline double roundSpeed(double speed)
{
    if (speed <= 0.0)
        return speed;
    return std::round(speed / SpeedStep) * SpeedStep;
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

} // namespace TransferDisplay
