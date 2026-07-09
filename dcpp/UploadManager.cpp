/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#include "UploadManager.h"

#include <cmath>

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "HashManager.h"
#include "AdcCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "File.h"
#include "extra/ipfilter.h"

namespace dcpp {

static const string UPLOAD_AREA = "Uploads";


UploadManager::UploadManager() noexcept : extra(0), lastGrant(0), running(0), limits(NULL), lastFreeSlots(-1) {
    ClientManager::getInstance()->addListener(this);
    TimerManager::getInstance()->addListener(this);
}

UploadManager::~UploadManager() {
    TimerManager::getInstance()->removeListener(this);
    ClientManager::getInstance()->removeListener(this);
    while(true) {
        {
            Lock l(cs);
            if(uploads.empty())
                break;
        }
        Thread::sleep(100);
    }
}

bool UploadManager::hasUpload ( UserConnection& aSource ) {
    Lock l(cs);
    if (!aSource.getSocket() || SETTING(ALLOW_SIM_UPLOADS))
        return false;

    for ( UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i ) {
        Upload* u = *i;
        const string l_srcip = aSource.getSocket()->getIp();
        const int64_t l_share = ClientManager::getInstance()->getBytesShared(aSource.getUser());

        if (u && u->getUserConnection().getSocket() &&
                l_srcip == u->getUserConnection().getSocket()->getIp() &&
                u->getUser() && l_share == ClientManager::getInstance()->getBytesShared(u->getUser())
                )
        {
            return true;
        }
    }

    return false;
}

void UploadManager::removeUpload(Upload* aUpload) {
    Lock l(cs);
    dcassert(find(uploads.begin(), uploads.end(), aUpload) != uploads.end());
    uploads.erase(remove(uploads.begin(), uploads.end(), aUpload), uploads.end());
    delete aUpload;
}

void UploadManager::reserveSlot(const HintedUser& aUser) {
    {
        Lock l(cs);
        reservedSlots.insert(aUser);
    }
    if(aUser.user->isOnline())
        ClientManager::getInstance()->connect(aUser, Util::toString(Util::rand()));
}

} // namespace dcpp
