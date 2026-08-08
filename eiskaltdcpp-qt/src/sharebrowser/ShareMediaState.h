/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QStringList>
#include <QVariantMap>

#include "dcpp/stdinc.h"
#include "dcpp/DirectoryListing.h"

/**
 * Optional-column presence for a share listing pane, plus TTHs that still need
 * ShareIndex media enrich (any media field empty).
 */
class ShareMediaState {
public:
    void reset();
    void noteFile(const dcpp::DirectoryListing::File *file);
    void noteEnrich(const QVariantMap &m);

    bool hasBitrate() const { return bitrate_; }
    bool hasResolution() const { return resolution_; }
    bool hasVideo() const { return video_; }
    bool hasAudio() const { return audio_; }
    bool hasDownloaded() const { return downloaded_; }
    bool hasShared() const { return shared_; }
    const QStringList &missingTths() const { return missing_; }

private:
    bool bitrate_ = false;
    bool resolution_ = false;
    bool video_ = false;
    bool audio_ = false;
    bool downloaded_ = false;
    bool shared_ = false;
    QStringList missing_;
};
