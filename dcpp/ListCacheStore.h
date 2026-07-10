/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "CID.h"

#include <cstdint>
#include <ctime>

namespace dcpp {

namespace ListCacheStore {

void load();
int64_t shareSize(const CID& cid);
time_t fetchTime(const CID& cid);
void setMeta(const CID& cid, int64_t shareSize, time_t when);

} // namespace ListCacheStore

} // namespace dcpp
