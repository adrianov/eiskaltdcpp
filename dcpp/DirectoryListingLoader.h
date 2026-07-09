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

#pragma once

#include "DirectoryListing.h"
#include "SimpleXMLReader.h"

namespace dcpp {

/** SAX callback that builds a DirectoryListing tree from FileListing XML. */
class ListLoader : public SimpleXMLReader::CallBack {
public:
    ListLoader(DirectoryListing::Directory* root, bool aUpdating);

    void startTag(const string& name, StringPairList& attribs, bool simple) override;
    void endTag(const string& name) override;

    const string& getBase() const { return base; }

private:
    DirectoryListing::Directory* cur;
    string base;
    bool inListing;
    bool updating;
    bool m_is_mediainfo_list;
    bool m_is_first_check_mediainfo_list;
};

} // namespace dcpp
