/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
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

namespace ClientManagerHubGuard {

bool sameHubUrl(const string& a, const string& b);
bool hasActiveHub(const string& url, const Client* exclude);
/** True if a connected hub matches url (host/port) or, when non-empty, hub name. */
bool hasActiveHub(const string& url, const string& name, const Client* exclude);

} // namespace ClientManagerHubGuard

} // namespace dcpp
