/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "typedefs.h"

namespace dcpp {

/** Digit-aware case-insensitive UTF-8 compare; "file1" before "file11".
 *  Paths with / or \\: fewer directory segments first, then natural order. Returns <0, 0, >0. */
int compareNatural(const string& a, const string& b);

} // namespace dcpp
