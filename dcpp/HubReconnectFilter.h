/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

namespace HubReconnectFilter {

constexpr int MAX_ATTEMPTS = 8;

bool shouldGiveUp(int attempts);
int delaySec(int attempts);
string delayLabel(int attempts);

} // namespace HubReconnectFilter

} // namespace dcpp
