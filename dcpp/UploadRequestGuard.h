/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"
#include "CriticalSection.h"

#include <deque>
#include <string>
#include <unordered_map>

namespace dcpp {

/**
 * Rejects peers that spam the same upload GET (any type/file/range) in a short
 * window. Keyed by remote IP when known (survives reconnect / nick change),
 * otherwise by CID.
 */
class UploadRequestGuard {
public:
    static UploadRequestGuard& getInstance() {
        static UploadRequestGuard instance;
        return instance;
    }

    /** True when this GET may proceed; false when the peer is abusing. */
    bool allow(const UserPtr& user, const string& remoteIp,
               const string& type, const string& file,
               int64_t start, int64_t bytes);

    void prune(uint64_t now);

private:
    UploadRequestGuard() = default;

    struct Hit {
        string fingerprint;
        deque<uint64_t> ticks;
        uint64_t bannedUntil = 0;
        bool loggedBan = false;
    };

    string peerKey(const UserPtr& user, const string& remoteIp) const;
    static string makeFingerprint(const string& type, const string& file,
                                  int64_t start, int64_t bytes);
    void dropStale(Hit& hit, uint64_t now) const;

    mutable CriticalSection cs;
    unordered_map<string, Hit> peers;
};

} // namespace dcpp
