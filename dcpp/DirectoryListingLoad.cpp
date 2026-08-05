/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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
#include "DirectoryListing.h"
#include "DirectoryListingLoader.h"

#include "BZUtils.h"
#include "File.h"
#include "FilteredFile.h"
#include "SettingsManager.h"
#include "SimpleXMLReader.h"
#include "Streams.h"

namespace dcpp {

bool DirectoryListing::loadFile(const string& path) {
    string actualPath;
    const string extIn = Util::getFileExt(path);
    if(Util::stricmp(extIn, ".bz2") != 0 && dcpp::File::getSize(path + ".bz2") != -1) {
        actualPath = path + ".bz2";
    }

    auto ext = Util::getFileExt(actualPath.empty() ? path : actualPath);
    bool hasEntries = false;

    {
        dcpp::File file(actualPath.empty() ? path : actualPath, dcpp::File::READ, dcpp::File::OPEN);
        if(Util::stricmp(ext, ".bz2") == 0) {
            FilteredInputStream<UnBZFilter, false> f(&file);
            loadXML(f, false, &hasEntries);
        } else if(Util::stricmp(ext, ".xml") == 0) {
            loadXML(file, false, &hasEntries);
        } else {
            throw Exception(_("Invalid file list extension (must be .xml or .bz2)"));
        }
    }
    return hasEntries;
}

string DirectoryListing::updateXML(const string& xml) {
    MemoryInputStream mis(xml);
    return loadXML(mis, true);
}

string DirectoryListing::loadXML(InputStream& is, bool updating, bool* hasEntries) {
    ListLoader ll(getRoot(), updating);

    SimpleXMLReader(&ll).parse(is, SETTING(MAX_FILELIST_SIZE) ? (size_t)SETTING(MAX_FILELIST_SIZE)*1024*1024 : 0);

    if(hasEntries)
        *hasEntries = ll.hasEntries();

    // Drop empty / nest-only complete dirs (keeps Incomplete placeholders).
    getRoot()->pruneEmptyDirs();

    return ll.getBase();
}

} // namespace dcpp
