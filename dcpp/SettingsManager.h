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

#pragma once

#include "Util.h"
#include "Speaker.h"
#include "Singleton.h"
#include "Exception.h"

namespace dcpp {

STANDARD_EXCEPTION(SearchTypeException);

class SimpleXML;

class SettingsManagerListener {
public:
    virtual ~SettingsManagerListener() { }
    template<int I> struct X { enum { TYPE = I }; };

    typedef X<0> Load;
    typedef X<1> Save;
    typedef X<2> SearchTypesChanged;

    virtual void on(Load, SimpleXML&) noexcept { }
    virtual void on(Save, SimpleXML&) noexcept { }
    virtual void on(SearchTypesChanged) noexcept { }
};

class SettingsManager : public Singleton<SettingsManager>, public Speaker<SettingsManagerListener>
{
public:
    typedef std::unordered_map<string, StringList> SearchTypes;
    typedef SearchTypes::iterator SearchTypesIter;
    typedef SearchTypes::const_iterator SearchTypesIterC;

    enum Types {
        TYPE_STRING,
        TYPE_INT,
        TYPE_INT64
    };

    static StringList connectionSpeeds;

    #include "SettingsManagerKeys.h"


    enum {
        INCOMING_DIRECT,
        INCOMING_FIREWALL_UPNP,
        INCOMING_FIREWALL_NAT,
        INCOMING_FIREWALL_PASSIVE
    };

    enum {  OUTGOING_DIRECT, OUTGOING_SOCKS5 };

    enum {  MAGNET_AUTO_SEARCH, MAGNET_AUTO_DOWNLOAD };

    const string& get(StrSetting key, bool useDefault = true) const;
    int get(IntSetting key, bool useDefault = true) const;
    bool getBool(IntSetting key, bool useDefault = true) const;
    int64_t get(Int64Setting key, bool useDefault = true) const;
    float get(FloatSetting key, bool useDefault = true) const;

    void set(StrSetting key, string const& value);
    void set(IntSetting key, int value);
    void set(IntSetting key, const string& value);
    void set(Int64Setting key, int64_t value);
    void set(IntSetting key, bool value);
    void set(Int64Setting key, const string& value);
    void set(FloatSetting key, float value);
    void set(FloatSetting key, double value);

    const string& getDefault(StrSetting key) const;
    int getDefault(IntSetting key) const;
    int64_t getDefault(Int64Setting key) const;
    float getDefault(FloatSetting key) const;

    void setDefault(StrSetting key, string const& value);
    void setDefault(IntSetting key, int value);
    void setDefault(Int64Setting key, int64_t value);
    void setDefault(FloatSetting key, float value);

    bool isDefault(size_t key) { return !isSet[key]; }

    void unset(size_t key) { isSet[key] = false; }

    void load() {
        Util::migrate(getConfigFile());
        load(getConfigFile());
    }
    void save() {
        save(getConfigFile());
    }

    void load(const string& aFileName);
    void save(const string& aFileName);

    bool getType(const char* name, int& n, Types& type) const;
    // Search types
    void validateSearchTypeName(const string& name) const;
    void setSearchTypeDefaults();
    void addSearchType(const string& name, const StringList& extensions, bool validated = false);
    void delSearchType(const string& name);
    void renameSearchType(const string& oldName, const string& newName);
    void modSearchType(const string& name, const StringList& extensions);

    const SearchTypes& getSearchTypes() const {
        return searchTypes;
    }
    const StringList& getExtensions(const string& name);

    const string parseCoreCmd(const string& cmd);
    bool parseCoreCmd(string& ret, const string& key, const string& value);

private:
    friend class Singleton<SettingsManager>;
    SettingsManager();
    virtual ~SettingsManager() { }

    void setDefaults();
    static string defaultHubListServers();

    static const string settingTags[SETTINGS_LAST+1];

    string strSettings[STR_LAST - STR_FIRST];
    int    intSettings[INT_LAST - INT_FIRST];
    int64_t int64Settings[INT64_LAST - INT64_FIRST];
    float floatSettings[FLOAT_LAST - FLOAT_FIRST];

    string strDefaults[STR_LAST - STR_FIRST];
    int    intDefaults[INT_LAST - INT_FIRST];
    int64_t int64Defaults[INT64_LAST - INT64_FIRST];
    float floatDefaults[FLOAT_LAST - FLOAT_FIRST];

    bool isSet[SETTINGS_LAST];

    static string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "DCPlusPlus.xml"; }

    // Search types
    SearchTypes searchTypes; // name, extlist

    SearchTypesIter getSearchType(const string& name);
};

// Shorthand accessor macros
#define SETTING(k) (SettingsManager::getInstance()->get(SettingsManager::k, true))
#define BOOLSETTING(k) (SettingsManager::getInstance()->getBool(SettingsManager::k, true))

} // namespace dcpp
