/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../CID.h"

#include <cstdint>
#include <ctime>
#include <string>

namespace dcpp {

using std::string;

/**
 * In-memory + ListCache.xml persistence for per-CID file-list metadata.
 * Thread-safe: map access under dataCs, XML I/O under fileCs; load() is once.
 */
namespace ListCacheStore {

/** Idempotent one-time XML load; safe from any thread. */
void load();
int64_t shareSize(const CID& cid);
int64_t fileSize(const CID& cid);
time_t fetchTime(const CID& cid);
void setMeta(const CID& cid, int64_t shareSize, int64_t fileSize, time_t when);

bool isCid(const string& cid);
/** FetchTime for base32 CID, or -1; caller must load() first. */
time_t cachedFetch(const string& cid);
bool eraseCid(const string& cid);
/** Erase cid only when FetchTime is in [0, cutoff); false if missing/fresh. */
bool eraseIfFetchedBefore(const string& cid, time_t cutoff);
/** Keep newer fetchTime when importing a legacy sidecar. */
void mergeMigrated(const string& cid, int64_t shareSize, time_t fetchTime);
/** Write ListCache.xml; false when I/O fails. */
bool persist();
/** Erase rows with FetchTime in [0, cutoff). */
bool eraseFetchedBefore(time_t cutoff);

} // namespace ListCacheStore

} // namespace dcpp
