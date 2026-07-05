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

class Client;

/** Hub text that denies search (e.g. class registration required). */
bool isSearchDeniedHubText(const string& message);
void noteSearchDenied(Client& client, const string& message);

} // namespace dcpp
