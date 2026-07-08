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

namespace TransferDisplay {

constexpr int SpeedSigFigs = 2;
constexpr int64_t TimeLeftMinBump = 5 * 60;

double roundSpeed(double speed);

inline int64_t smoothTimeLeft(int64_t displayed, int64_t actual)
{
    if (actual < 0 || displayed < 0)
        return actual;
    if (actual <= displayed)
        return actual;
    if (!displayed || actual >= displayed + TimeLeftMinBump || actual * 2 >= displayed * 3)
        return actual;
    return displayed;
}

} // namespace TransferDisplay
