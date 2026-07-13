/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"

#include "SearchQuery.h"

#include "StringTokenizer.h"

namespace dcpp {

namespace {

/** ASCII letter or any non-ASCII byte (UTF-8 letters, e.g. Cyrillic). */
bool hasAlpha(const string& word) {
    for(unsigned char c : word) {
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80)
            return true;
    }
    return false;
}

/** Short alnum suffix with a letter (mkv, mp3, 7z) — not season tags or years. */
bool looksLikeExt(const string& ext) {
    if(ext.empty() || ext.size() > 5)
        return false;
    bool letter = false;
    for(unsigned char c : ext) {
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            letter = true;
        else if(c < '0' || c > '9')
            return false;
    }
    return letter;
}

string stripExt(const string& name) {
    const auto dot = name.rfind('.');
    if(dot == string::npos || dot == 0)
        return name;
    if(!looksLikeExt(name.substr(dot + 1)))
        return name;
    return name.substr(0, dot);
}

} // namespace

string SearchQuery::limitHubSearch(const string& aString, size_t maxLen) {
    if(aString.size() <= maxLen)
        return aString;

    size_t cut = aString.rfind(' ', maxLen);
    if(cut == string::npos)
        return aString.substr(0, maxLen);

    while(cut > 0 && aString[cut - 1] == ' ')
        --cut;

    return aString.substr(0, cut);
}

string SearchQuery::filenameWords(const string& aFileName, size_t wordCount) {
    const string base = stripExt(aFileName);

    string normalized;
    normalized.reserve(base.size());

    for(char c : base) {
        if(c == '.' || c == '_' || c == '-' || c == '\t' || c == '\r' || c == '\n')
            normalized.push_back(' ');
        else
            normalized.push_back(c);
    }

    StringTokenizer<string> tok(normalized, ' ');
    string result;
    size_t added = 0;

    for(const auto& word : tok.getTokens()) {
        if(word.empty() || !hasAlpha(word))
            continue;
        if(!result.empty())
            result.push_back(' ');
        result += word;
        if(++added >= wordCount)
            break;
    }

    return limitHubSearch(result);
}

} // namespace dcpp
