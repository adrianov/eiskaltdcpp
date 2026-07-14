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
#include "typedefs.h"

namespace dcpp {

namespace PeerConnectHub {

enum HubOutcome {
    UNKNOWN = 0,
    SUCCESS = 1,
    FAILURE = 2
};

HubOutcome get(const UserPtr& user, const string& hub);
void rememberSuccess(const UserPtr& user, const string& hub);
void rememberFailure(const UserPtr& user, const string& hub);
void sortSources(HintedUserList& sources);
void load();
void save();

} // namespace PeerConnectHub

} // namespace dcpp
