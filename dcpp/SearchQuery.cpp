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
    string normalized;
    normalized.reserve(aFileName.size());

    for(char c : aFileName) {
        if(c == '.' || c == '_' || c == '-')
            normalized.push_back(' ');
        else if(c == '\t' || c == '\r' || c == '\n')
            normalized.push_back(' ');
        else
            normalized.push_back(c);
    }

    StringTokenizer<string> tok(normalized, ' ');
    string result;
    size_t added = 0;

    for(const auto& word : tok.getTokens()) {
        if(word.empty())
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
