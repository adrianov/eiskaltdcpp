/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "NmdcHub.h"

namespace dcpp {

bool NmdcHub::isNickLike(const string& nick) {
    if(nick.empty() || nick.size() > 64)
        return false;
    for(unsigned char c : nick) {
        if(c <= 32 || c == ':' || c == '|' || c == '$' || c == '<' || c == '>')
            return false;
    }
    return true;
}

bool NmdcHub::hasControlChars(const string& s) {
    for(unsigned char c : s) {
        if(c < 32)
            return true;
    }
    return false;
}

string NmdcHub::validateMessage(string tmp, bool reverse) {
    string::size_type i = 0;

    if(reverse) {
        while( (i = tmp.find("&#36;", i)) != string::npos) {
            tmp.replace(i, 5, "$");
            i++;
        }
        i = 0;
        while( (i = tmp.find("&#124;", i)) != string::npos) {
            tmp.replace(i, 6, "|");
            i++;
        }
        i = 0;
        while( (i = tmp.find("&amp;", i)) != string::npos) {
            tmp.replace(i, 5, "&");
            i++;
        }
    } else {
        i = 0;
        while( (i = tmp.find("&amp;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find("&#36;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find("&#124;", i)) != string::npos) {
            tmp.replace(i, 1, "&amp;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find('$', i)) != string::npos) {
            tmp.replace(i, 1, "&#36;");
            i += 4;
        }
        i = 0;
        while( (i = tmp.find('|', i)) != string::npos) {
            tmp.replace(i, 1, "&#124;");
            i += 5;
        }
    }
    return tmp;
}

} // namespace dcpp
