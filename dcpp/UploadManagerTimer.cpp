/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "UploadManager.h"

#include "Upload.h"
#include "UserConnection.h"

namespace dcpp {

void UploadManager::on(TimerManagerListener::Second, uint64_t) noexcept {
    Lock l(cs);
    UploadList ticks;

    for (auto u : uploads) {
        if (u->getPos() > 0) {
            ticks.push_back(u);
            u->tick();
            u->getUserConnection().updateTransferSpeed(u->getStartPos() + u->getPos());
        }
    }

    // Only progressing uploads need UI refresh; idle slots stay until Starting/Complete/Failed.
    if (!ticks.empty())
        fire(UploadManagerListener::Tick(), ticks);
}

} // namespace dcpp
