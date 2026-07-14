/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "ADLSearch.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "DebugManager.h"
#include "DownloadManager.h"
#include "FavoriteManager.h"
#include "FinishedManager.h"
#include "HashManager.h"
#include "IncomingPortCheck.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "PeerConnectHub.h"
#include "QueueManager.h"
#include "ResourceManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#ifdef LUA_SCRIPT
#include "ScriptManager.h"
#endif
#include "ShareManager.h"
#include "ThrottleManager.h"
#include "UploadManager.h"

#include "extra/ipfilter.h"
#include "extra/dyndns.h"
#ifdef WITH_DHT
#include "dht/DHT.h"
#endif

namespace dcpp {

void shutdown() {
    DebugManager::deleteInstance();
    DynDNS::deleteInstance();
#ifdef WITH_DHT
    dht::DHT::deleteInstance();
#endif
    ThrottleManager::getInstance()->shutdown();
    TimerManager::getInstance()->shutdown();
    HashManager::getInstance()->shutdown();

    ConnectionManager::getInstance()->shutdown();
    MappingManager::getInstance()->close();

    // Drop HTTP sockets before waiting — owners are deleted later.
    FavoriteManager::getInstance()->abortHttp();
    IncomingPortCheck::deleteInstance();

    const bool socketsStopped = BufferedSocket::waitShutdown();
#ifdef LUA_SCRIPT
    // After sockets stop: UserConnection::send still calls into Lua until then.
    if(socketsStopped)
        ScriptManager::deleteInstance();
#endif
    QueueManager::getInstance()->removeUserLists();
    QueueManager::getInstance()->saveQueue(true);
    ClientManager::getInstance()->saveUsers();
    PeerConnectHub::save();
    if (IPFilter::getInstance()) {
        IPFilter::getInstance()->shutdown();
    }
    SettingsManager::getInstance()->save();
    if(!socketsStopped)
        return;

    MappingManager::deleteInstance();
    ConnectivityManager::deleteInstance();
    ADLSearchManager::deleteInstance();
    FinishedManager::deleteInstance();
    ShareManager::deleteInstance();
    CryptoManager::deleteInstance();
    ThrottleManager::deleteInstance();
    DownloadManager::deleteInstance();
    UploadManager::deleteInstance();
    QueueManager::deleteInstance();
    ConnectionManager::deleteInstance();
    SearchManager::deleteInstance();
    FavoriteManager::deleteInstance();
    ClientManager::deleteInstance();
    HashManager::deleteInstance();
    LogManager::deleteInstance();
    SettingsManager::deleteInstance();
    TimerManager::deleteInstance();
    ResourceManager::deleteInstance();

#ifdef _WIN32
    ::WSACleanup();
#endif
}

} // namespace dcpp
