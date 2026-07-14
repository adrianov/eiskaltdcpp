/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "Download.h"
#include "ListCache.h"
#include "Transfer.h"
#include "UserConnection.h"

namespace dcpp {

void QueueManager::putDownload(Download* aDownload, bool finished) noexcept {
    HintedUserList getConn;
    string fl_fname;
    HintedUser fl_user(UserPtr(), Util::emptyString);
    int fl_flag = 0;

    unique_ptr<Download> d(aDownload);
    aDownload = nullptr;

    const auto queued = ConnectionManager::getInstance()->queuedDownloadUsers();
    {
        Lock l(cs);
        delete d->getFile();
        d->setFile(0);
        putDownloadBody(d.get(), finished, getConn, fl_fname, fl_user, fl_flag, queued);
    }

    for(auto& i: getConn)
        ConnectionManager::getInstance()->getDownloadConnection(i);

    if(!fl_fname.empty())
        processList(fl_fname, fl_user, fl_flag);
}

} // namespace dcpp
