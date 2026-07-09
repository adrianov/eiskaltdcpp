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
#include "CID.h"
#include "StringTokenizer.h"

namespace dcpp {

void SettingsManager::load(string const& aFileName)
{
    try {
        SimpleXML xml;

        xml.fromXML(File(aFileName, File::READ, File::OPEN).read());

        xml.resetCurrentChild();

        xml.stepIn();

        if(xml.findChild("Settings"))
        {
            xml.stepIn();

            int i;

            for(i=STR_FIRST; i<STR_LAST; i++)
            {
                const string& attr = settingTags[i];
                dcassert(attr.find("SENTRY") == string::npos);

                if(xml.findChild(attr))
                    set(StrSetting(i), xml.getChildData());
                xml.resetCurrentChild();
            }
            for(i=INT_FIRST; i<INT_LAST; i++)
            {
                const string& attr = settingTags[i];
                dcassert(attr.find("SENTRY") == string::npos);

                if(xml.findChild(attr))
                    set(IntSetting(i), Util::toInt(xml.getChildData()));
                xml.resetCurrentChild();
            }
            for(i=FLOAT_FIRST; i<FLOAT_LAST; i++)
            {
                const string& attr = settingTags[i];
                dcassert(attr.find("SENTRY") == string::npos);

                if(xml.findChild(attr))
                    set(FloatSetting(i), Util::toInt(xml.getChildData()) / 1000.);
                xml.resetCurrentChild();
            }
            for(i=INT64_FIRST; i<INT64_LAST; i++)
            {
                const string& attr = settingTags[i];
                dcassert(attr.find("SENTRY") == string::npos);

                if(xml.findChild(attr))
                    set(Int64Setting(i), Util::toInt64(xml.getChildData()));
                xml.resetCurrentChild();
            }

            xml.stepOut();
        }

        xml.resetCurrentChild();
        if(xml.findChild("SearchTypes")) {
            try {
                searchTypes.clear();
                xml.stepIn();
                while(xml.findChild("SearchType")) {
                    const string& extensions = xml.getChildData();
                    if(extensions.empty()) {
                        continue;
                    }
                    const string& name = xml.getChildAttrib("Id");
                    if(name.empty()) {
                        continue;
                    }
                    searchTypes[name] = StringTokenizer<string>(extensions, ';').getTokens();
                }
                xml.stepOut();
            } catch(const SimpleXMLException&) {
                setSearchTypeDefaults();
            }
        }

        if(SETTING(PRIVATE_ID).length() != 39 || !CID(SETTING(PRIVATE_ID))) {
            set(PRIVATE_ID, CID::generate().toBase32());
        }

        double v = Util::toDouble(SETTING(CONFIG_VERSION));
        // if(v < 0.x) { // Fix old settings here }

        if(v <= 0.674) {

            // Formats changed, might as well remove these...
            unset(LOG_FORMAT_POST_DOWNLOAD);
            unset(LOG_FORMAT_POST_UPLOAD);
            unset(LOG_FORMAT_MAIN_CHAT);
            unset(LOG_FORMAT_PRIVATE_CHAT);
            unset(LOG_FORMAT_STATUS);
            unset(LOG_FORMAT_SYSTEM);
            unset(LOG_FILE_MAIN_CHAT);
            unset(LOG_FILE_STATUS);
            unset(LOG_FILE_PRIVATE_CHAT);
            unset(LOG_FILE_UPLOAD);
            unset(LOG_FILE_DOWNLOAD);
            unset(LOG_FILE_SYSTEM);
        }

        if(v <= 2.0402) {
            // Merge newly introduced default hublist servers into stored lists.
            // Bump the version when a new default hublist entry is added.
            string lists = get(HUBLIST_SERVERS);
            StringTokenizer<string> t(getDefault(HUBLIST_SERVERS), ';');

            for(auto& i: t.getTokens()) {
                if(lists.find(i) == string::npos)
                    lists += ";" + i;
            }
            set(HUBLIST_SERVERS, lists);
        }

        if(SETTING(SET_MINISLOT_SIZE) < 64)
            set(SET_MINISLOT_SIZE, 64);
        if(SETTING(AUTODROP_INTERVAL) < 1)
            set(AUTODROP_INTERVAL, 1);
        if(SETTING(AUTODROP_ELAPSED) < 1)
            set(AUTODROP_ELAPSED, 1);
        if(SETTING(AUTO_SEARCH_LIMIT) > 5)
            set(AUTO_SEARCH_LIMIT, 5);
        else if(SETTING(AUTO_SEARCH_LIMIT) < 1)
            set(AUTO_SEARCH_LIMIT, 1);
        if(SETTING(MAX_FILELIST_SIZE) < 1024)
            set(MAX_FILELIST_SIZE, 1024);

#ifdef _DEBUG
        set(PRIVATE_ID, CID::generate().toBase32());
#endif
        setDefault(UDP_PORT, SETTING(TCP_PORT));

        File::ensureDirectory(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));

        fire(SettingsManagerListener::Load(), xml);

        xml.stepOut();

    } catch(const Exception&) {
        if(!CID(SETTING(PRIVATE_ID)))
            set(PRIVATE_ID, CID::generate().toBase32());
    }
    if (SETTING(DHT_KEY).length() != 39 || !CID(SETTING(DHT_KEY)))
        set(DHT_KEY, CID::generate().toBase32());
}

} // namespace dcpp
