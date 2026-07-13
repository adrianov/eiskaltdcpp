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
#include "ShareManager.h"

#include "HashManager.h"

namespace dcpp {

void ShareManager::removeDir(const string& realPath) noexcept {
    if (realPath.empty())
        return;

    string path = realPath;
    if (path.back() != PATH_SEPARATOR)
        path += PATH_SEPARATOR;

    bool shareRoot = false;
    {
        Lock l(cs);
        shareRoot = (shares.find(path) != shares.end());
    }
    if (shareRoot) {
        removeDirectory(path);
        return;
    }

    HashManager::getInstance()->stopHashing(path);

    Lock l(cs);
    Directory::Ptr d = getDirectory(path + "x");
    if (!d || !d->getParent())
        return;

    d->getParent()->directories.erase(d->getName());
    rebuildIndices();
    setDirty();
    forceXmlRefresh = true;
}

} // namespace dcpp
