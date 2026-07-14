/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "FavoriteManager.h"

#include "ClientManager.h"

namespace dcpp {

void FavoriteManager::addFavoriteUser(const UserPtr& aUser) {
    Lock l(cs);
    if(users.find(aUser->getCID()) == users.end()) {
        StringList urls = ClientManager::getInstance()->getHubs(aUser->getCID(), Util::emptyString);
        StringList nicks = ClientManager::getInstance()->getNicks(aUser->getCID(), Util::emptyString);

        /// @todo make this an error probably...
        if(urls.empty())
            urls.push_back(Util::emptyString);
        if(nicks.empty())
            nicks.push_back(Util::emptyString);

        auto i = users.emplace(aUser->getCID(), FavoriteUser(aUser, nicks[0], urls[0])).first;
        fire(FavoriteManagerListener::UserAdded(), i->second);
        save();
    }
}

void FavoriteManager::removeFavoriteUser(const UserPtr& aUser) {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i != users.end()) {
        fire(FavoriteManagerListener::UserRemoved(), i->second);
        users.erase(i);
        save();
    }
}

std::string FavoriteManager::getUserURL(const UserPtr& aUser) const {
    Lock l(cs);
    auto i = users.find(aUser->getCID());
    if(i != users.end()) {
        const FavoriteUser& fu = i->second;
        return fu.getUrl();
    }
    return Util::emptyString;
}

void FavoriteManager::addFavorite(const FavoriteHubEntry& aEntry) {
    if(getFavoriteHub(aEntry.getServer()) != favoriteHubs.end()) {
        return;
    }
    auto f = new FavoriteHubEntry(aEntry);
    favoriteHubs.push_back(f);
    fire(FavoriteManagerListener::FavoriteAdded(), f);
    save();
}

void FavoriteManager::removeFavorite(FavoriteHubEntry* entry) {
    auto i = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
    if(i == favoriteHubs.end()) {
        return;
    }

    fire(FavoriteManagerListener::FavoriteRemoved(), entry);
    favoriteHubs.erase(i);
    delete entry;
    save();
}

bool FavoriteManager::isFavoriteHub(const std::string& url) {
    if(getFavoriteHub(url) != favoriteHubs.end()) {
        return true;
    }
    return false;
}

bool FavoriteManager::addFavoriteDir(const string& aDirectory, const string & aName){
    string path = aDirectory;

    if( path[ path.length() -1 ] != PATH_SEPARATOR )
        path += PATH_SEPARATOR;

    for(auto& i: favoriteDirs) {
        if((Util::strnicmp(path, i.first, i.first.size()) == 0) && (Util::strnicmp(path, i.first, path.size()) == 0)) {
            return false;
        }
        if(Util::stricmp(aName, i.second) == 0) {
            return false;
        }
    }
    favoriteDirs.emplace_back(aDirectory, aName);
    save();
    return true;
}

bool FavoriteManager::removeFavoriteDir(const string& aName) {
    string d(aName);

    if(d[d.length() - 1] != PATH_SEPARATOR)
        d += PATH_SEPARATOR;

    for(auto j = favoriteDirs.begin(); j != favoriteDirs.end(); ++j) {
        if(Util::stricmp(j->first.c_str(), d.c_str()) == 0) {
            favoriteDirs.erase(j);
            save();
            return true;
        }
    }
    return false;
}

bool FavoriteManager::renameFavoriteDir(const string& aName, const string& anotherName) {

    for(auto& j: favoriteDirs) {
        if(Util::stricmp(j.second.c_str(), aName.c_str()) == 0) {
            j.second = anotherName;
            save();
            return true;
        }
    }
    return false;
}



} // namespace dcpp
