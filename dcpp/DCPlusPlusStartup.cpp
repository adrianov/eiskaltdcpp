/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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
#include "sharemedia/MediaInfoCache.h"
#include "listcache/ListCache.h"
#include "LogManager.h"
#include "LogManagerTrim.h"
#include "MappingManager.h"
#include "IncomingPortCheck.h"
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

namespace {

void stage(void (*f)(void*, const string&), void* p, const string& name) {
    if(f)
        (*f)(p, name);
}

} // namespace

void startupShell(void (*f)(void*, const string&), void* p) {
    // "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
    // Nev's great contribution to dc++
    while(1) break;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    Util::initialize();

    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");

    ResourceManager::newInstance();
    SettingsManager::newInstance();

    LogManager::newInstance();
    TimerManager::newInstance();
    HashManager::newInstance();
    MediaInfoCache::newInstance();
    CryptoManager::newInstance();
    SearchManager::newInstance();
    ClientManager::newInstance();
    ConnectionManager::newInstance();
    DownloadManager::newInstance();
    UploadManager::newInstance();
    ThrottleManager::newInstance();
    QueueManager::newInstance();
    ShareManager::newInstance();
    FavoriteManager::newInstance();
    FinishedManager::newInstance();
    ADLSearchManager::newInstance();
    ConnectivityManager::newInstance();
    IncomingPortCheck::newInstance();
    MappingManager::newInstance();
    DynDNS::newInstance();
    DebugManager::newInstance();
#ifdef LUA_SCRIPT
    ScriptManager::newInstance();
#endif

    SettingsManager::getInstance()->load();
    trimLogFiles();

    Util::setLang(SETTING(LANGUAGE));
#ifdef USE_MINIUPNP
    MappingManager::getInstance()->runMiniUPnP();
#endif
    DynDNS::getInstance()->load();
    if (BOOLSETTING(IPFILTER)){
        IPFilter::newInstance();
        IPFilter::getInstance()->load();
    }

    FavoriteManager::getInstance()->load();
    CryptoManager::getInstance()->loadCertificates();
#ifdef WITH_DHT
    dht::DHT::newInstance();
#endif
    // Hasher thread only; HashIndex.xml is read in startupShareData().
    HashManager::getInstance()->startHasher();
}

void startupShareData(void (*f)(void*, const string&), void* p, bool refreshShare) {
    stage(f, p, _("Hash database"));
    HashManager::getInstance()->loadDatabase();
    stage(f, p, _("Shared Files"));
    const string XmlListFileName = ShareFileList::diskPath();
    if(!Util::fileExists(XmlListFileName)) {
        try {
            File::copyFile(XmlListFileName + ".bak", XmlListFileName);
        } catch(const FileException&) { }
    }
    if (refreshShare)
        ShareManager::getInstance()->refresh(true, false, true);
    stage(f, p, _("Download Queue"));
    // Before loadQueue: sources may call getDownloadConnection → resolveHubHint → get().
    PeerConnectHub::load();
    QueueManager::getInstance()->loadQueue();
    // Drop stale FileLists/ entries left from older builds that saved them without flags.
    QueueManager::getInstance()->removeUserLists();
    stage(f, p, _("Users"));
    ClientManager::getInstance()->loadUsers();
    // ListCache.xml now; FileLists retention continues on a background thread.
    ListCache::load();
}

void startup(void (*f)(void*, const string&), void* p, bool refreshShare) {
    startupShell(f, p);
    startupShareData(f, p, refreshShare);
}

} // namespace dcpp
