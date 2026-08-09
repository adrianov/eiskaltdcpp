/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../typedefs.h"

namespace dcpp {

/**
 * Gate before hashing and before storing a TTH: skip paths still open for
 * write elsewhere, and reject results whose size/mtime drifted mid-pass.
 */
class HashFileGate {
public:
    /** Another process likely holds the path open for writing. */
    static bool writerOpen(const string& path) noexcept;

    /** Size and mtime still match the snapshot from hash start. */
    static bool metaStable(const string& path, int64_t size, uint32_t mtime) noexcept;

    /** Commit allowed: not writer-held and meta unchanged. */
    static bool acceptHash(const string& path, int64_t size, uint32_t mtime) noexcept {
        return !writerOpen(path) && metaStable(path, size, mtime);
    }
};

} // namespace dcpp
