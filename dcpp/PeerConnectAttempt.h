/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "ConnectionQueueItem.h"

namespace dcpp {

/**
 * One outgoing download connect attempt for a queue item.
 *
 * Owns the item bookkeeping an attempt needs: attempt tick and counter, hub hint
 * resolution, direct or reverse connect, and the rollback that keeps the item
 * WAITING when the hub refuses to carry the request. ConnectionManager stays the
 * owner of the queue itself — it picks the identity, fires listener events and
 * decides what happens to an item that gets no attempt.
 *
 * Sibling of DownloadRetryPolicy: this makes an attempt, that one cleans up after
 * a failed one.
 */
class PeerConnectAttempt {
public:
    PeerConnectAttempt(ConnectionQueueItem* cqi, uint64_t tick) noexcept;

    /** Order a queue snapshot so peers that can grant a slot now are tried first. */
    static void preferFreeSlots(ConnectionQueueItem::List& order);

    /** Peer-side gates: no pending upload slot wait, and the queue wants this peer. */
    static bool ready(const HintedUser& user);

    /** Ask the hub to connect. True marks the item CONNECTING; false leaves it WAITING. */
    bool start() const;

private:
    ConnectionQueueItem* const cqi;
    const uint64_t tick;
};

} // namespace dcpp
