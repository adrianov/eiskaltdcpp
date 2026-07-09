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

#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "version.h"

namespace dcpp {

void SettingsManager::save(string const& aFileName) {

    SimpleXML xml;
    xml.addTag("DCPlusPlus");
    xml.stepIn();
    xml.addTag("Settings");
    xml.stepIn();

    int i;
    string type("type"), curType("string");

    for(i=STR_FIRST; i<STR_LAST; i++)
    {
        if(i == CONFIG_VERSION) {
            xml.addTag(settingTags[i], VERSIONFLOAT);
            xml.addChildAttrib(type, curType);
        } else if(isSet[i]) {
            xml.addTag(settingTags[i], get(StrSetting(i), false));
            xml.addChildAttrib(type, curType);
        }
    }

    curType = "int";
    for(i=INT_FIRST; i<INT_LAST; i++)
    {
        if(isSet[i]) {
            xml.addTag(settingTags[i], get(IntSetting(i), false));
            xml.addChildAttrib(type, curType);
        }
    }
    for(i=FLOAT_FIRST; i<FLOAT_LAST; i++)
    {
        if(isSet[i]) {
            xml.addTag(settingTags[i], static_cast<int>(get(FloatSetting(i), false) * 1000.));
            xml.addChildAttrib(type, curType);
        }
    }
    curType = "int64";
    for(i=INT64_FIRST; i<INT64_LAST; i++)
    {
        if(isSet[i])
        {
            xml.addTag(settingTags[i], get(Int64Setting(i), false));
            xml.addChildAttrib(type, curType);
        }
    }
    xml.stepOut();

    xml.addTag("SearchTypes");
    xml.stepIn();
    for(auto& i: searchTypes) {
        xml.addTag("SearchType", Util::toString(";", i.second));
        xml.addChildAttrib("Id", i.first);
    }
    xml.stepOut();

    fire(SettingsManagerListener::Save(), xml);

    try {
        File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
        BufferedOutputStream<false> f(&out);
        f.write(SimpleXML::utf8Header);
        xml.toXML(&f);
        f.flush();
        out.close();
        File::deleteFile(aFileName);
        File::renameFile(aFileName + ".tmp", aFileName);
    } catch(const FileException&) {
        // ...
    }
}

} // namespace dcpp
