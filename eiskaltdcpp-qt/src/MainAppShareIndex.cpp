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

#include "dcpp/ClientManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/TimerManager.h"

#include <memory>

using namespace dcpp;

namespace {

/** Batches hub user arrivals before matching their saved shares to the queue. */
class ShareIndexOnline : private ClientManagerListener, private TimerManagerListener
{
public:
    void start()
    {
        ClientManager::getInstance()->addListener(this);
        TimerManager::getInstance()->addListener(this);
    }

    void stop()
    {
        TimerManager::getInstance()->removeListener(this);
        ClientManager::getInstance()->removeListener(this);
    }

private:
    void on(ClientManagerListener::UserConnected, const UserPtr &user) noexcept override
    {
        if (!user)
            return;
        Lock l(cs);
        pending[user->getCID()] = user;
    }

    void on(TimerManagerListener::Second, uint64_t) noexcept override
    {
        UserList users;
        {
            Lock l(cs);
            users.reserve(pending.size());
            for (const auto &entry : pending)
                users.push_back(entry.second);
            pending.clear();
        }
        if (!users.empty() && ShareIndex::getInstance())
            ShareIndex::getInstance()->matchQueue(users);
    }

    CriticalSection cs;
    unordered_map<CID, UserPtr> pending;
};

std::unique_ptr<ShareIndexOnline> onlineMatcher;

} // namespace

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
    onlineMatcher.reset(new ShareIndexOnline());
    onlineMatcher->start();
}

void stopShareIndex()
{
    if (onlineMatcher) {
        onlineMatcher->stop();
        onlineMatcher.reset();
    }
    if (ShareIndex::getInstance())
        ShareIndex::getInstance()->stopWrites();
    ShareIndexListListener::deleteInstance();
    ReciprocalList::deleteInstance();
    ShareIndex::deleteInstance();
}
