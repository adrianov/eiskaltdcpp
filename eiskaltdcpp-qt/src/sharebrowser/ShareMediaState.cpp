/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sharebrowser/ShareMediaState.h"
#include "WulforUtil.h"

using namespace dcpp;

void ShareMediaState::reset()
{
    bitrate_ = resolution_ = video_ = audio_ = false;
    downloaded_ = shared_ = false;
    missing_.clear();
}

void ShareMediaState::noteFile(const DirectoryListing::File *file)
{
    if (!file)
        return;
    const MediaInfo &mi = file->mediaInfo;
    if (mi.bitrate > 0)
        bitrate_ = true;
    if (!mi.resolution.empty())
        resolution_ = true;
    if (!mi.video_info.empty())
        video_ = true;
    if (!mi.audio_info.empty())
        audio_ = true;
    if (file->getHit() > 0)
        downloaded_ = true;
    if (file->getTS() > 0)
        shared_ = true;
    // Any empty media field: ShareIndex may still fill the rest.
    if (mi.bitrate == 0 || mi.resolution.empty()
            || mi.video_info.empty() || mi.audio_info.empty()) {
        const QString tth = _q(file->getTTH().toBase32());
        if (!tth.isEmpty())
            missing_ << tth;
    }
}

void ShareMediaState::noteEnrich(const QVariantMap &m)
{
    if (m.value(QStringLiteral("bitrate")).toInt() > 0)
        bitrate_ = true;
    if (!m.value(QStringLiteral("resolution")).toString().isEmpty())
        resolution_ = true;
    if (!m.value(QStringLiteral("video")).toString().isEmpty())
        video_ = true;
    if (!m.value(QStringLiteral("audio")).toString().isEmpty())
        audio_ = true;
}
