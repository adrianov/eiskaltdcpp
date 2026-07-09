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
#include "dcpp/UploadManager.h"
#include "dcpp/Singleton.h"

/** When a peer finishes downloading our file list, queue theirs for ShareIndex. */
class ReciprocalList :
        public dcpp::Singleton<ReciprocalList>,
        private dcpp::UploadManagerListener
{
    friend class dcpp::Singleton<ReciprocalList>;

public:
    void start();
    void stop();

private:
    ReciprocalList() = default;
    ~ReciprocalList() override;

    void on(dcpp::UploadManagerListener::Complete, dcpp::Upload *ul) noexcept override;
    void maybeFetch(const dcpp::HintedUser &peer);
};
