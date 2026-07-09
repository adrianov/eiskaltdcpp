/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MainAppShareIndex.h"

#include "ShareIndex.h"
#include "ReciprocalList.h"
#include "ShareIndexListListener.h"

#include "dcpp/SettingsManager.h"

void startShareIndex()
{
    ShareIndex::newInstance();
    // Open on the write worker: large WAL/DB must not block or abort the UI thread.
    ShareIndex::getInstance()->openAsync();
    // Keep downloaded file lists so share-size cache and ShareIndex can reuse them.
    dcpp::SettingsManager::getInstance()->set(dcpp::SettingsManager::KEEP_LISTS, true);
    ReciprocalList::newInstance();
    ReciprocalList::getInstance()->start();
    ShareIndexListListener::newInstance();
    ShareIndexListListener::getInstance()->start();
}

void stopShareIndex()
{
    if (ShareIndex::getInstance())
        ShareIndex::getInstance()->stopWrites();
    ShareIndexListListener::deleteInstance();
    ReciprocalList::deleteInstance();
    ShareIndex::deleteInstance();
}

void startShareIndexBackfill()
{
    ShareIndex::getInstance()->ingestCachedLists();
}
