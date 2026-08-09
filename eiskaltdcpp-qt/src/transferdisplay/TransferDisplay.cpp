/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "transferdisplay/TransferDisplay.h"

#include "dcpp/stdinc.h"
#include "dcpp/format.h"
#include "dcpp/SettingsManager.h"

#include <cmath>

using namespace dcpp;

namespace TransferDisplay {

namespace {

double unitSize()
{
    return SETTING(APP_UNIT_BASE) < 2 ? 1024.0 : 1000.0;
}

bool binaryNames()
{
    return SETTING(APP_UNIT_BASE) == 0;
}

struct Scaled {
    double value = 0.0;
    int unit = 0; // 0=B … 5=PB
};

Scaled pickUnit(double bytes)
{
    const double u = unitSize();
    Scaled s{bytes, 0};
    if (bytes < u)
        return s;

    double factor = u;
    s.unit = 1;
    s.value = bytes / factor;
    while (s.unit < 5 && bytes >= factor * u) {
        factor *= u;
        ++s.unit;
        s.value = bytes / factor;
    }
    return s;
}

/** Ones digit 0 when showing ≥ 10 in KB+; B stays exact. */
double stepSize(int unit, double value)
{
    if (unit <= 0)
        return 1.0;
    return value >= 10.0 ? 10.0 : 1.0;
}

Scaled fitStep(Scaled s)
{
    const double u = unitSize();
    for (;;) {
        const double step = stepSize(s.unit, s.value);
        s.value = std::round(s.value / step) * step;
        if (s.unit > 0 && s.unit < 5 && s.value >= u) {
            s.value /= u;
            ++s.unit;
            continue;
        }
        return s;
    }
}

double asBytes(Scaled s)
{
    double factor = 1.0;
    const double u = unitSize();
    for (int i = 0; i < s.unit; ++i)
        factor *= u;
    return s.value * factor;
}

const char *unitLabel(int unit, bool bin)
{
    static const char *binLabel[] = {
        N_("B"), N_("KiB"), N_("MiB"), N_("GiB"), N_("TiB"), N_("PiB")
    };
    static const char *decLabel[] = {
        N_("B"), N_("KB"), N_("MB"), N_("GB"), N_("TB"), N_("PB")
    };
    return (bin ? binLabel : decLabel)[unit];
}

} // namespace

double roundBytes(double bytes)
{
    if (bytes <= 0.0)
        return bytes;
    return asBytes(fitStep(pickUnit(bytes)));
}

QString formatBytes(int64_t bytes)
{
    char buf[128];
    if (bytes <= 0) {
        snprintf(buf, sizeof(buf), "%d %s", 0, _("B"));
        return QString::fromUtf8(buf);
    }
    const Scaled s = fitStep(pickUnit(static_cast<double>(bytes)));
    snprintf(buf, sizeof(buf), "%d %s",
             static_cast<int>(std::lround(s.value)), _(unitLabel(s.unit, binaryNames())));
    return QString::fromUtf8(buf);
}

} // namespace TransferDisplay
