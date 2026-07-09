/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "HubReconnectFilter.h"

#include "format.h"

namespace dcpp {
namespace HubReconnectFilter {

namespace {

constexpr int DELAYS_SEC[] = {
    60,      // 1 min
    120,     // 2 min
    300,     // 5 min
    900,     // 15 min
    3600,    // 1 hour
    10800,   // 3 hours
    43200,   // 12 hours
    86400    // 1 day
};

} // namespace

bool shouldGiveUp(int attempts) {
    return attempts >= MAX_ATTEMPTS;
}

int delaySec(int attempts) {
    if(attempts <= 0)
        return DELAYS_SEC[0];
    return DELAYS_SEC[min(attempts, MAX_ATTEMPTS) - 1];
}

string delayLabel(int attempts) {
    switch(min(max(attempts, 1), MAX_ATTEMPTS)) {
    case 1: return _("1 minute");
    case 2: return _("2 minutes");
    case 3: return _("5 minutes");
    case 4: return _("15 minutes");
    case 5: return _("1 hour");
    case 6: return _("3 hours");
    case 7: return _("12 hours");
    default: return _("1 day");
    }
}

} // namespace HubReconnectFilter
} // namespace dcpp
