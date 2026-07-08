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

double roundBytes(double bytes)
{
    if (bytes <= 0.0)
        return bytes;

    const double unit = SETTING(APP_UNIT_BASE) < 2 ? 1024.0 : 1000.0;
    double inUnit = bytes;
    double factor = 1.0;

    for (double b = unit; bytes >= b; b *= unit) {
        inUnit = bytes / b;
        factor = b;
    }

    const double scale = std::pow(10.0, std::floor(std::log10(inUnit)) - (ByteSigFigs - 1));
    return std::round(inUnit / scale) * scale * factor;
}

} // namespace TransferDisplay
