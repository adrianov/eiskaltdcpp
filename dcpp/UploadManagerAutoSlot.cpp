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
#include "FavoriteManager.h"

namespace dcpp {

bool UploadManager::getAutoSlot() {
    /** A 0 in settings means disable */
    if(SETTING(MIN_UPLOAD_SPEED) == 0)
        return false;
    /** Only grant one slot per 30 sec */
    if(GET_TICK() < getLastGrant() + 30*1000)
        return false;
    /** Grant if upload speed is less than the threshold speed */
    return getRunningAverage() < (SETTING(MIN_UPLOAD_SPEED)*1024);
}

} // namespace dcpp
