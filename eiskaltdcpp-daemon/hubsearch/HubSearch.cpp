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

#include "stdafx.h"
#include "hubsearch/HubSearch.h"

#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/StringTokenizer.h"
#include "dcpp/Text.h"
#include "dcpp/Util.h"
#include "dcpp/format.h"

using namespace dcpp;

StringList HubSearch::hubsToQuery(const string& huburls, const StringList& allHubs)
{
    if (!huburls.empty())
        return StringTokenizer<string>(huburls, ";").getTokens();
    return allHubs;
}

string HubSearch::positiveQuery(const TStringList& terms)
{
    string q;
    for (const auto& item : terms) {
        if (!item.empty() && item[0] != '-')
            q += item + ' ';
    }
    if (!q.empty())
        q.pop_back();
    return q;
}

int64_t HubSearch::sizeBytes(double size, int sizetype)
{
    static const double mul[] = { 1.0, 1024.0, 1024.0 * 1024.0, 1024.0 * 1024.0 * 1024.0 };
    if (sizetype >= 1 && sizetype <= 3)
        size *= mul[sizetype];
    return static_cast<int64_t>(size);
}

int HubSearch::typeAndExts(int searchtype, StringList& exts)
{
    if (searchtype <= SearchManager::TYPE_ANY || searchtype >= SearchManager::TYPE_LAST)
        return SearchManager::TYPE_ANY;
    if (searchtype == SearchManager::TYPE_DIRECTORY || searchtype == SearchManager::TYPE_TTH)
        return searchtype;
    try {
        exts = SearchManager::getTypeExtensions(searchtype);
        return searchtype;
    } catch (const SearchTypeException&) {
        return SearchManager::TYPE_ANY;
    }
}

bool HubSearch::start(const string& search, int searchtype, int sizemode, int sizetype,
                      double size, const string& huburls, const StringList& allHubs)
{
    if (search.empty())
        return false;
    StringList hubs = hubsToQuery(huburls, allHubs);
    if (hubs.empty())
        return false;

    terms = StringTokenizer<string>(search, ' ').getTokens();
    const int64_t bytes = sizeBytes(size, sizetype);
    auto mode = static_cast<SearchManager::SizeModes>(sizemode);
    if (!bytes)
        mode = SearchManager::SIZE_DONTCARE;

    StringList exts;
    const int ftype = typeAndExts(searchtype, exts);
    token = Util::toString(Util::rand());
    isHash = (ftype == SearchManager::TYPE_TTH);

    SearchManager::getInstance()->search(hubs, positiveQuery(terms), bytes,
            SearchManager::TypeModes(ftype), mode, token, exts);
    return true;
}

void HubSearch::add(const SearchResultPtr& result)
{
    if (result)
        byHub[result->getHubURL()].push_back(result);
}

void HubSearch::append(vector<StringMap>& out, const string& huburl) const
{
    auto it = byHub.find(huburl);
    if (it == byHub.end())
        return;
    for (const auto& sr : it->second) {
        if (skip(sr))
            continue;
        StringMap row;
        parse(sr, row);
        out.push_back(row);
    }
}

void HubSearch::clearHub(const string& huburl)
{
    byHub[huburl].clear();
}

bool HubSearch::skip(const SearchResultPtr& result) const
{
    if (terms.empty() || !result)
        return true;
    if (!result->getToken().empty() && token != result->getToken())
        return true;
    if (isHash) {
        return result->getType() != SearchResult::TYPE_FILE
                || TTHValue(Text::fromT(terms[0])) != result->getTTH();
    }
    for (const auto& j : terms) {
        if (j.empty())
            continue;
        if (j[0] != '-' && Util::findSubString(result->getFile(), j) == string::npos)
            return true;
        if (j[0] == '-' && j.size() != 1
                && Util::findSubString(result->getFile(), j.substr(1)) != string::npos)
            return true;
    }
    return false;
}

string HubSearch::nativePath(const string& path)
{
    string str = path;
    for (auto& ch : str) {
#ifdef _WIN32
        if (ch == '/')
            ch = '\\';
#else
        if (ch == '\\')
            ch = '/';
#endif
    }
    return str;
}

void HubSearch::parse(const SearchResultPtr& result, StringMap& row)
{
    if (result->getType() == SearchResult::TYPE_FILE) {
        string file = nativePath(result->getFile());
        if (file.rfind('/') == string::npos) {
            row["Filename"] = file;
        } else {
            row["Filename"] = Util::getFileName(file);
            row["Path"] = Util::getFilePath(file);
        }
        row["File Order"] = "f" + row["Filename"];
        row["Type"] = Util::getFileExt(row["Filename"]);
        if (!row["Type"].empty() && row["Type"][0] == '.')
            row["Type"].erase(0, 1);
        row["Size"] = Util::formatBytes(result->getSize());
        row["Exact Size"] = Util::formatExactSize(result->getSize());
        row["Icon"] = "icon-file";
        row["Shared"] = Util::toString(ShareManager::getInstance()->isTTHShared(result->getTTH()));
    } else {
        string path = nativePath(result->getFile());
        row["Filename"] = Util::getLastDir(path) + PATH_SEPARATOR;
        row["Path"] = Util::getFilePath(path.substr(0, path.length() - 1));
        if (row["Path"].find("/") == string::npos)
            row["Path"] = "";
        row["File Order"] = "d" + row["Filename"];
        row["Type"] = _("Directory");
        row["Icon"] = "icon-directory";
        row["Shared"] = "0";
        if (result->getSize() > 0) {
            row["Size"] = Util::formatBytes(result->getSize());
            row["Exact Size"] = Util::formatExactSize(result->getSize());
        }
    }

    row["Nick"] = Util::toString(ClientManager::getInstance()->getNicks(result->getUser()->getCID(), result->getHubURL()));
    row["CID"] = result->getUser()->getCID().toBase32();
    row["Slots"] = result->getSlotString();
    row["Connection"] = ClientManager::getInstance()->getConnection(result->getUser()->getCID());
    row["Hub"] = result->getHubName().empty() ? result->getHubURL().c_str() : result->getHubName().c_str();
    row["Hub URL"] = result->getHubURL();
    row["IP"] = result->getIP();
    row["Real Size"] = Util::toString(result->getSize());
    if (result->getType() == SearchResult::TYPE_FILE)
        row["TTH"] = result->getTTH().toBase32();
    row["Slots Order"] = Util::toString(-1000 * result->getFreeSlots() - result->getSlots());
    row["Free Slots"] = Util::toString(result->getFreeSlots());
}
