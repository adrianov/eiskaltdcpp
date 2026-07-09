/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "dcpp/stdinc.h"
#include "dcpp/QueueManager.h"
#include "dcpp/UploadManager.h"
#include "dcpp/Singleton.h"

/**
 * Silent peer file-list fetch for ShareIndex.
 * Triggers: peer downloaded our full list, or became a source on an active download.
 * Reuses ListCache when share size matches; otherwise downloads at most once per day,
 * with a concurrent queue cap and a short per-peer cooldown.
 */
class ReciprocalList :
        public dcpp::Singleton<ReciprocalList>,
        private dcpp::UploadManagerListener,
        private dcpp::QueueManagerListener
{
    friend class dcpp::Singleton<ReciprocalList>;

public:
    void start();
    void stop();

private:
    ReciprocalList() = default;
    ~ReciprocalList() override;

    void on(dcpp::UploadManagerListener::Complete, dcpp::Upload *ul) noexcept override;
    void on(dcpp::QueueManagerListener::SourceAdded, dcpp::QueueItem *item,
            const dcpp::HintedUser &user) noexcept override;
    void maybeFetch(const dcpp::HintedUser &peer);
};
