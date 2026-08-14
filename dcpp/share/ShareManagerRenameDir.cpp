/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

bool ShareManager::renameDir(const string& realPath, const string& newName) noexcept {
    if (realPath.empty() || newName.empty())
        return false;

    string path = realPath;
    if (path.back() != PATH_SEPARATOR)
        path += PATH_SEPARATOR;

    const string newPath = Util::getFilePath(path.substr(0, path.size() - 1))
            + newName + PATH_SEPARATOR;
    if (newPath == path && newName == Util::getLastDir(path))
        return false;

    {
        Lock l(cs);
        if (shares.find(path) != shares.end()) {
            const string oldVirt = shares.find(path)->second;
            const string newVirt = validateVirtual(newName);
            auto existing = getByVirtual(newVirt);
            if (existing != directories.end() && Util::stricmp(oldVirt, newVirt) != 0)
                return false;
            int virtCount = 0;
            for (const auto& s : shares) {
                if (Util::stricmp(s.second, oldVirt) == 0)
                    ++virtCount;
            }
            if (virtCount > 1)
                return false;
        } else {
            Directory::Ptr d = getDirectory(path + "x");
            if (!d || !d->getParent())
                return false;
            Directory *parent = d->getParent();
            if (parent->directories.find(newName) != parent->directories.end()
                    && Util::stricmp(d->getName(), newName) != 0)
                return false;
        }
    }

    HashManager::getInstance()->stopHashing(path);
    HashManager::getInstance()->renameDir(path, newPath);

    Lock l(cs);
    auto shareIt = shares.find(path);
    if (shareIt != shares.end()) {
        const string oldVirt = shareIt->second;
        const string newVirt = validateVirtual(newName);
        shares.erase(shareIt);
        auto j = getByVirtual(oldVirt);
        if (j != directories.end())
            (*j)->setName(newVirt);
        shares[newPath] = newVirt;
        rebuildIndices();
        setDirty();
        fileList.forceRefresh();
        return true;
    }

    Directory::Ptr d = getDirectory(path + "x");
    if (!d || !d->getParent())
        return false;

    Directory *parent = d->getParent();
    parent->directories.erase(d->getName());
    d->setName(newName);
    parent->directories.emplace(newName, d);
    rebuildIndices();
    setDirty();
    fileList.forceRefresh();
    return true;
}

} // namespace dcpp
