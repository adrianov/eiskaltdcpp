/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "hub/HubUserCounts.h"

namespace dcpp {

string HubUserCounts::format() const {
    char buf[128];
    return string(buf, snprintf(buf, sizeof(buf), "%ld/%ld/%ld",
                                static_cast<long>(normal),
                                static_cast<long>(registered),
                                static_cast<long>(op)));
}

} // namespace dcpp
