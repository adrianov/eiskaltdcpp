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

#include "FinishedManager.h"

#include "FinishedItem.h"
#include "DownloadManager.h"
#include "QueueManager.h"
#include "UploadManager.h"

namespace dcpp {

FinishedManager::FinishedManager() {
    DownloadManager::getInstance()->addListener(this);
    UploadManager::getInstance()->addListener(this);
    QueueManager::getInstance()->addListener(this);
}

FinishedManager::~FinishedManager() {
    DownloadManager::getInstance()->removeListener(this);
    UploadManager::getInstance()->removeListener(this);
    QueueManager::getInstance()->removeListener(this);

    clearDLs();
    clearULs();
}

Lock FinishedManager::lock() {
    return Lock(cs);
}

const FinishedManager::MapByFile& FinishedManager::getMapByFile(bool upload) const {
    return upload ? ULByFile : DLByFile;
}

const FinishedManager::MapByUser& FinishedManager::getMapByUser(bool upload) const {
    return upload ? ULByUser : DLByUser;
}

} // namespace dcpp
