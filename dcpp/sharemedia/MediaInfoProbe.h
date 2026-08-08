/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 * MediaInfoLib probe adapted from FlylinkDC++ getMediaInfo.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "MediaInfo.h"

#include <string>

namespace dcpp {

/** Thread-safe local file probe via MediaInfoLib (BR/WH/MV/MA). */
class MediaInfoProbe
{
public:
    /** Open path and fill out; false on open/parse failure. */
    static bool scan(const std::string& path, MediaInfo& out);
};

} // namespace dcpp
