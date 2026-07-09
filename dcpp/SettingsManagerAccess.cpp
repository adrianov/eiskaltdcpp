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

namespace dcpp {

const string& SettingsManager::get(StrSetting key, bool useDefault) const {
    return (isSet[key] || !useDefault) ? strSettings[key - STR_FIRST] : strDefaults[key - STR_FIRST];
}

int SettingsManager::get(IntSetting key, bool useDefault) const {
    return (isSet[key] || !useDefault) ? intSettings[key - INT_FIRST] : intDefaults[key - INT_FIRST];
}

bool SettingsManager::getBool(IntSetting key, bool useDefault) const {
    return (get(key, useDefault) != 0);
}

int64_t SettingsManager::get(Int64Setting key, bool useDefault) const {
    return (isSet[key] || !useDefault) ? int64Settings[key - INT64_FIRST] : int64Defaults[key - INT64_FIRST];
}

float SettingsManager::get(FloatSetting key, bool useDefault) const {
    return (isSet[key] || !useDefault) ? floatSettings[key - FLOAT_FIRST] : floatDefaults[key - FLOAT_FIRST];
}

void SettingsManager::set(StrSetting key, string const& value) {
    if(((key == DESCRIPTION) || (key == NICK)) && (value.size() > 35)) {
        strSettings[key - STR_FIRST] = value.substr(0, 35);
    } else {
        strSettings[key - STR_FIRST] = value;
    }
    isSet[key] = !value.empty();
}

void SettingsManager::set(IntSetting key, int value) {
    if((key == SLOTS) && (value <= 0)) {
        value = 1;
    }
    intSettings[key - INT_FIRST] = value;
    isSet[key] = true;
}

void SettingsManager::set(IntSetting key, const string& value) {
    if(value.empty()) {
        intSettings[key - INT_FIRST] = 0;
        isSet[key] = false;
    } else {
        intSettings[key - INT_FIRST] = Util::toInt(value);
        isSet[key] = true;
    }
}

void SettingsManager::set(Int64Setting key, int64_t value) {
    int64Settings[key - INT64_FIRST] = value;
    isSet[key] = true;
}

void SettingsManager::set(IntSetting key, bool value) {
    set(key, (int)value);
}

void SettingsManager::set(Int64Setting key, const string& value) {
    if(value.empty()) {
        int64Settings[key - INT64_FIRST] = 0;
        isSet[key] = false;
    } else {
        int64Settings[key - INT64_FIRST] = Util::toInt64(value);
        isSet[key] = true;
    }
}

void SettingsManager::set(FloatSetting key, float value) {
    floatSettings[key - FLOAT_FIRST] = value;
    isSet[key] = true;
}

void SettingsManager::set(FloatSetting key, double value) {
    // yes, we loose precision here, but we're gonna loose even more when saving to the XML file...
    floatSettings[key - FLOAT_FIRST] = static_cast<float>(value);
    isSet[key] = true;
}

const string& SettingsManager::getDefault(StrSetting key) const {
    return strDefaults[key - STR_FIRST];
}

int SettingsManager::getDefault(IntSetting key) const {
    return intDefaults[key - INT_FIRST];
}

int64_t SettingsManager::getDefault(Int64Setting key) const {
    return int64Defaults[key - INT64_FIRST];
}

float SettingsManager::getDefault(FloatSetting key) const {
    return floatDefaults[key - FLOAT_FIRST];
}

void SettingsManager::setDefault(StrSetting key, string const& value) {
    strDefaults[key - STR_FIRST] = value;
}

void SettingsManager::setDefault(IntSetting key, int value) {
    intDefaults[key - INT_FIRST] = value;
}

void SettingsManager::setDefault(Int64Setting key, int64_t value) {
    int64Defaults[key - INT64_FIRST] = value;
}

void SettingsManager::setDefault(FloatSetting key, float value) {
    floatDefaults[key - FLOAT_FIRST] = value;
}

} // namespace dcpp
