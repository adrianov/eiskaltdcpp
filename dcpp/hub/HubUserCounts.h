/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../Atomic.h"

namespace dcpp {

/** Global / last-known hub user class counts (normal / registered / op). */
struct HubUserCounts {
private:
    typedef Atomic<std::int32_t> atomic_counter_t;
public:
    typedef std::int32_t value_type;
    HubUserCounts(value_type n = 0, value_type r = 0, value_type o = 0) :
        normal(n), registered(r), op(o) { }
    atomic_counter_t normal;
    atomic_counter_t registered;
    atomic_counter_t op;

    int total() const { return normal + registered + op; }
    string format() const;
};

} // namespace dcpp
