/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "forward.h"
#include "typedefs.h"

namespace dcpp {

/**
 * Retry decision for one failed download connection.
 *
 * Classifies the failure from the socket state and the error text, then applies
 * the resulting backoff to the connection queue item: error and slot-wait
 * counters, last attempt tick, TLS retry mode and hub failure memory. The item
 * is left WAITING so the connection timer can try again, possibly on another
 * hub. ConnectionManager keeps the item and fires all listener events.
 *
 * Failure kinds:
 *  - slot wait       closed after handshake without granting a slot
 *  - soft reconnect  peer sent a file, then dropped while idle or between files
 *  - protocol error  park until the give-up cooldown expires
 *  - plain failure   count one error toward give-up (TLS mismatch included, so
 *                    the other handshake mode gets a turn)
 */
class DownloadRetryPolicy {
public:
    DownloadRetryPolicy(const UserConnection* source, const string& error, bool protocolError) noexcept;

    /** Peer closed after a completed handshake — a slot wait, not a connect failure. */
    bool postHandshakeClose() const { return postClose; }

    /** Log the failure unless it is a slot wait or a repeated TLS retry. */
    void logFail(int errors) const;

    /** Update counters and backoff; leaves the item WAITING. */
    void apply(ConnectionQueueItem* cqi) const;

    /** Park the item: no attempts until the give-up cooldown expires. */
    static void markGiveUp(ConnectionQueueItem* cqi, int attempts, bool slotWait);

    /** True when the peer never answered on any hub; caller drops item and sources. */
    static bool dropUnreachable(ConnectionQueueItem* cqi);

private:
    const UserConnection* source;
    const string error;
    const bool protocolError;
    const bool tlsMismatch;
    const bool postClose;
};

} // namespace dcpp
