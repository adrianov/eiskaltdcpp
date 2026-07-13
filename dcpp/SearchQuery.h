/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"

namespace dcpp {

/** Hub search strings: many hubs reject queries longer than 50 characters. */
class SearchQuery {
public:
    static constexpr size_t MAX_LEN = 50;

    static string limitHubSearch(const string& aString, size_t maxLen = MAX_LEN);
    /** Up to wordCount letter-bearing name parts; drops real extension and digit/punct-only tokens. */
    static string filenameWords(const string& aFileName, size_t wordCount = 3);
};

} // namespace dcpp
