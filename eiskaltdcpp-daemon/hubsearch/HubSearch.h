/***************************************************************************
*                                                                         *
*   Copyright (C) 2009-2010  Alexandr Tkachev <tka4ev@gmail.com>          *
*   Copyright (C) 2020 Boris Pek <tehnick-8@yandex.ru>                    *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include "dcpp/SearchManager.h"
#include "dcpp/SearchResult.h"

#include <string>
#include <unordered_map>
#include <vector>

/** Last daemon hub search: query filter, per-hub hits, and StringMap rows for RPC. */
class HubSearch
{
public:
    bool start(const string& search, int searchtype, int sizemode, int sizetype,
               double size, const string& huburls, const StringList& allHubs);
    void add(const SearchResultPtr& result);
    void append(vector<StringMap>& out, const string& huburl) const;
    void clearHub(const string& huburl);

private:
    bool skip(const SearchResultPtr& result) const;
    static void parse(const SearchResultPtr& result, StringMap& row);
    static string nativePath(const string& path);
    static string positiveQuery(const TStringList& terms);
    static int64_t sizeBytes(double size, int sizetype);
    static int typeAndExts(int searchtype, StringList& exts);
    static StringList hubsToQuery(const string& huburls, const StringList& allHubs);

    TStringList terms;
    string token;
    bool isHash = false;
    std::unordered_map<string, SearchResultList> byHub;
};
