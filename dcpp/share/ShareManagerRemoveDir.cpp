/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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

namespace {

string withSlash(string path)
{
    if (!path.empty() && path.back() != PATH_SEPARATOR)
        path += PATH_SEPARATOR;
    return path;
}

} // namespace

bool ShareManager::isNestedShareDir(const string& realPath) const noexcept {
    if (realPath.empty())
        return false;

    const string path = withSlash(realPath);
    Lock l(cs);
    bool nested = false;
    bool isRoot = false;
    bool hasInner = false;
    for (const auto& s : shares) {
        const string& root = s.first;
        if (Util::stricmp(path, root) == 0)
            isRoot = true;
        else if (root.size() > path.size() && Util::strnicmp(root, path, path.size()) == 0)
            hasInner = true;
        else if (path.size() > root.size() && Util::strnicmp(path, root, root.size()) == 0)
            nested = true;
    }
    return nested && !isRoot && !hasInner;
}

void ShareManager::removeDir(const string& realPath) noexcept {
    if (realPath.empty() || !isNestedShareDir(realPath))
        return;

    string path = withSlash(realPath);

    HashManager::getInstance()->stopHashing(path);

    Lock l(cs);
    Directory::Ptr d = getDirectory(path + "x");
    if (!d || !d->getParent())
        return;

    d->getParent()->directories.erase(d->getName());
    rebuildIndices();
    setDirty();
    fileList.forceRefresh();
}

} // namespace dcpp
