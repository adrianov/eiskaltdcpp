/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferDisplay.h"

#include "dcpp/stdinc.h"
#include "dcpp/SettingsManager.h"

#include <cmath>

using namespace dcpp;

namespace TransferDisplay {
namespace {

double roundSigFigs(double value, int sigfigs)
{
    if (value <= 0.0)
        return value;
    const double decade = std::floor(std::log10(value));
    const double scale = std::pow(10.0, decade - (sigfigs - 1));
    return std::round(value / scale) * scale;
}

void speedUnit(double speed, uint8_t base, double &inUnit, double &factor)
{
    const uint16_t a = (base < 2 ? 1024 : 1000);
    const double b = static_cast<double>(a);

    inUnit = speed;
    factor = 1.0;

    if (speed < b)
        return;

    inUnit = speed / b;
    factor = b;

    if (speed < b * b)
        return;

    inUnit = speed / (b * b);
    factor = b * b;

    if (speed < b * b * b)
        return;

    inUnit = speed / (b * b * b);
    factor = b * b * b;

    if (speed < b * b * b * b)
        return;

    inUnit = speed / (b * b * b * b);
    factor = b * b * b * b;

    inUnit = speed / (b * b * b * b * b);
    factor = b * b * b * b * b;
}

} // namespace

double roundSpeed(double speed)
{
    if (speed <= 0.0)
        return speed;

    double inUnit = speed;
    double factor = 1.0;
    speedUnit(speed, SETTING(APP_UNIT_BASE), inUnit, factor);
    return roundSigFigs(inUnit, SpeedSigFigs) * factor;
}

} // namespace TransferDisplay
