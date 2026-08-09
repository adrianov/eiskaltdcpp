/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../HashManagerListener.h"
#include "../MerkleTree.h"
#include "../typedefs.h"

namespace dcpp {

class QueueManager;

/**
 * Removes download-queue items whose target already matches on disk
 * (same path, size, and TTH) — e.g. finished by another application.
 */
class LocalMatch : private HashManagerListener {
public:
    explicit LocalMatch(QueueManager& queue);
    ~LocalMatch();

    /** True when path exists with matching size and a cached TTH root. */
    static bool matches(const string& target, int64_t size, const TTHValue& tth) noexcept;
    /** Sweep the queue; hash size-matched files when TTH is not cached yet. */
    void sweep() noexcept;

private:
    QueueManager& queue;

    void on(HashManagerListener::TTHDone, const string& fileName, const TTHValue& root) noexcept;
};

} // namespace dcpp
