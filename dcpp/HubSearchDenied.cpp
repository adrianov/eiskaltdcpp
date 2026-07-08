/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "HubSearchDenied.h"

#include "Client.h"
#include "Util.h"

namespace dcpp {

bool isSearchDeniedHubText(const string& message) {
    if(message.empty())
        return false;
    if(Util::findSubString(message, "can't search") != string::npos)
        return true;
    if(Util::findSubString(message, "cannot search") != string::npos)
        return true;
    if(Util::findSubString(message, "can not search") != string::npos)
        return true;
    if(Util::findSubString(message, "максимальная длина поиска") != string::npos)
        return true;
    if(Util::findSubString(message, "maximum search length") != string::npos)
        return true;
    if(Util::findSubString(message, "max search length") != string::npos)
        return true;
    if(Util::findSubString(message, "search length") != string::npos &&
            Util::findSubString(message, "50") != string::npos)
        return true;
    if(Util::findSubString(message, "search") != string::npos &&
            Util::findSubString(message, "registered with class") != string::npos &&
            (Util::findSubString(message, "must") != string::npos ||
             Util::findSubString(message, "need") != string::npos ||
             Util::findSubString(message, "require") != string::npos ||
             Util::findSubString(message, "denied") != string::npos ||
             Util::findSubString(message, "not allowed") != string::npos ||
             Util::findSubString(message, "unable") != string::npos))
        return true;
    return false;
}

void noteSearchDenied(Client& client, const string& message) {
    if(!client.getSearchBlocked() && isSearchDeniedHubText(message))
        client.setSearchBlocked(true);
}

} // namespace dcpp
