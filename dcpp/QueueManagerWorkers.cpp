/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
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
#include "QueueManagerWorkers.h"

#include "Exception.h"
#include "File.h"
#include "HashManager.h"
#include "MerkleCheckOutputStream.h"
#include "QueueManager.h"
#include "Segment.h"

namespace dcpp {

void FileMover::moveFile(const string& source, const string& target) {
    Lock l(cs);
    files.emplace_back(source, target);
    if(!active) {
        active = true;
        start();
    }
}

int FileMover::run() {
    while(true) {
        FilePair next;
        {
            Lock l(cs);
            if(files.empty()) {
                active = false;
                return 0;
            }
            next = files.back();
            files.pop_back();
        }

        QueueManager::moveFile_(next.first, next.second);
    }
    return 0;
}

} // namespace dcpp
