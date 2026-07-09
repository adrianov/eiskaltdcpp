/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2026 EiskaltDC++ developers
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

#include "SettingsManager.h"

#include "AdcHub.h"
#include "SearchManager.h"
#include "Util.h"
#include "StringTokenizer.h"
#include "format.h"

namespace dcpp {

void SettingsManager::validateSearchTypeName(const string& name) const {
    if(name.empty() || (name.size() == 1 && name[0] >= '1' && name[0] <= '6')) {
        throw SearchTypeException(_("Invalid search type name"));
    }
    for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type) {
        if(SearchManager::getTypeStr(type) == name) {
            throw SearchTypeException(_("This search type already exists"));
        }
    }
}

void SettingsManager::setSearchTypeDefaults() {
    searchTypes.clear();

    // for conveniency, the default search exts will be the same as the ones defined by SEGA.
    const auto& searchExts = AdcHub::getSearchExts();
    for(size_t i = 0, n = searchExts.size(); i < n; ++i)
        searchTypes[string(1, '1' + i)] = searchExts[i];

    fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::addSearchType(const string& name, const StringList& extensions, bool validated) {
    if(!validated) {
        validateSearchTypeName(name);
    }

    if(searchTypes.find(name) != searchTypes.end()) {
        throw SearchTypeException(_("This search type already exists"));
    }

    searchTypes[name] = extensions;
    fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::delSearchType(const string& name) {
    validateSearchTypeName(name);
    searchTypes.erase(name);
    fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::renameSearchType(const string& oldName, const string& newName) {
    validateSearchTypeName(newName);
    StringList exts = getSearchType(oldName)->second;
    addSearchType(newName, exts, true);
    searchTypes.erase(oldName);
}

void SettingsManager::modSearchType(const string& name, const StringList& extensions) {
    getSearchType(name)->second = extensions;
    fire(SettingsManagerListener::SearchTypesChanged());
}

const StringList& SettingsManager::getExtensions(const string& name) {
    return getSearchType(name)->second;
}

SettingsManager::SearchTypesIter SettingsManager::getSearchType(const string& name) {
    SearchTypesIter ret = searchTypes.find(name);
    if(ret == searchTypes.end()) {
        throw SearchTypeException(_("No such search type"));
    }
    return ret;
}

bool SettingsManager::getType(const char* name, int& n, Types &type) const {
    for(n = 0; n < INT64_LAST; n++) {
        if (strcmp(settingTags[n].c_str(), name) == 0) {
            if (n < STR_LAST) {
                type = TYPE_STRING;
                return true;
            } else if (n < INT_LAST) {
                type= TYPE_INT;
                return true;
            } else {
                type = TYPE_INT64;
                return true;
            }
        }
    }
    return false;
}

const std::string SettingsManager::parseCoreCmd(const string& cmd) {
    StringTokenizer<string> sl(cmd, ' ');
    if (sl.getTokens().size() == 1) {
        string ret;
        bool b = parseCoreCmd(ret, sl.getTokens().at(0), Util::emptyString);
        return (!b ? _("Error: setting not found!") : _("Core setting ") + sl.getTokens().at(0) + ": " + ret);
    } else if (sl.getTokens().size() >= 2) {
        string tmp = cmd.substr(sl.getTokens().at(0).size()+1);
        string ret;
        bool b = parseCoreCmd(ret, sl.getTokens().at(0), tmp);
        return (!b ? _("Error: setting not found!") : _("Change core setting ") + sl.getTokens().at(0) + _(" to ") + tmp);
    }
    return Util::emptyString;
}

bool SettingsManager::parseCoreCmd(string& ret, const string& key, const string& value) {
    if (key.empty()) {
        return false;
    }

    int n;
    SettingsManager::Types type;
    getType(key.c_str(), n, type);
    if (type == SettingsManager::TYPE_INT) {
        if (!value.empty()) {
            int i = atoi(value.c_str());
            set((SettingsManager::IntSetting)n,i);
        } else if (value.empty())
            ret = Util::toString(get((SettingsManager::IntSetting)n,false));
    }
    else if (type == SettingsManager::TYPE_STRING) {
        if (!value.empty())
            set((SettingsManager::StrSetting)n, value);
        else if (value.empty())
            ret = get((SettingsManager::StrSetting)n,false);
    } else
        return false;
    return true;
}

} // namespace dcpp
