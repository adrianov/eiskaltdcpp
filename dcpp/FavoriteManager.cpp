/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "FavoriteManager.h"

#include "ClientManager.h"
#include "File.h"
#include "SettingsManager.h"
#include "StringTokenizer.h"
#include "Util.h"

namespace dcpp {

FavoriteManager::FavoriteManager() : lastId(0), useHttp(false), running(false), c(nullptr), lastServer(0), listType(TYPE_NORMAL), dontSave(false) {
    SettingsManager::getInstance()->addListener(this);
    ClientManager::getInstance()->addListener(this);

    File::ensureDirectory(Util::getHubListsPath());
}

FavoriteManager::~FavoriteManager() {
    ClientManager::getInstance()->removeListener(this);
    SettingsManager::getInstance()->removeListener(this);
    abortHttp();

    for_each(favoriteHubs.begin(), favoriteHubs.end(), DeleteFunction());
}

StringList FavoriteManager::getHubLists() {
    StringTokenizer<string> lists(SETTING(HUBLIST_SERVERS), ';');
    return lists.getTokens();
}

FavoriteHubEntryList::iterator FavoriteManager::getFavoriteHub(const string& aServer) {
    for(FavoriteHubEntryList::iterator i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
        if(Util::stricmp((*i)->getServer(), aServer) == 0) {
            return i;
        }
    }
    return favoriteHubs.end();
}

void FavoriteManager::on(UserUpdated, const OnlineUser& user) noexcept {
    userUpdated(user);
}

void FavoriteManager::on(UserDisconnected, const UserPtr& user) noexcept {
    Lock l(cs);

    auto i = users.find(user->getCID());
    if (i != users.end())
    {
        i->second.setLastSeen(GET_TIME());
        fire(FavoriteManagerListener::StatusChanged(), i->second);
        save();
    }
}

void FavoriteManager::on(UserConnected, const UserPtr& user) noexcept {
    Lock l(cs);

    auto i = users.find(user->getCID());
    if (i != users.end())
        fire(FavoriteManagerListener::StatusChanged(), i->second);
}

string FavoriteManager::getConfigFile() {
    return Util::getPath(Util::PATH_USER_CONFIG) + "Favorites.xml";
}

} // namespace dcpp
