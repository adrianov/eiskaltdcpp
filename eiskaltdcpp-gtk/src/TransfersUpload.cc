/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "TransfersUpload.hh"

#include <glib/gi18n.h>
#include <algorithm>
#include <unordered_map>

#include <dcpp/File.h>
#include <dcpp/TimerManager.h>
#include <dcpp/Util.h>

#include "TransfersDisplay.hh"

using namespace std;
using namespace dcpp;

namespace {

static const uint64_t UPLOAD_UI_INTERVAL_MS = 250;
static unordered_map<string, uint64_t> uploadTickTimes;

int64_t uploadDiskSize(const Upload *ul) {
    if (ul->getType() != Transfer::TYPE_FILE)
        return -1;
    return File::getSize(ul->getPath());
}

} // namespace

namespace TransfersUpload {

UploadUiState uploadState(const Upload *ul) {
    UploadUiState s;
    const int64_t startPos = ul->getStartPos();
    const int64_t pos = ul->getPos();
    const int64_t segmentSize = ul->getSize();
    const int64_t diskSize = uploadDiskSize(ul);

    // Use full-file offsets only when File::getSize succeeds.
    if (diskSize > 0) {
        s.fileSize = diskSize;
        s.sent = startPos + pos;
        if (s.sent > s.fileSize)
            s.sent = s.fileSize;
    } else {
        s.fileSize = segmentSize > 0 ? segmentSize : pos;
        s.sent = pos;
    }
    s.continuing = startPos > 0;
    s.fileDone = diskSize > 0 && startPos + segmentSize >= diskSize;
    return s;
}

string uploadTickKey(const Upload *ul) {
    return ul->getUser()->getCID().toBase32() + "|" + ul->getPath();
}

bool shouldRefreshUploadUi(const string& key) {
    const uint64_t now = GET_TICK();
    const auto it = uploadTickTimes.find(key);
    if (it != uploadTickTimes.end() && now - it->second < UPLOAD_UI_INTERVAL_MS)
        return false;
    uploadTickTimes[key] = now;
    return true;
}

void clearUploadUiThrottle(const string& key) {
    uploadTickTimes.erase(key);
}

void setUploadParams(StringMap& params, Upload* ul, const UploadUiState& s) {
    params["Size"] = Util::toString(s.fileSize);
    params["Download Position"] = Util::toString(s.sent);
    const double progress = s.fileSize > 0
        ? std::min(100.0, std::max(0.0, s.sent * 100.0 / s.fileSize)) : 0.0;
    params["Progress Hidden"] = Util::toString(static_cast<int>(progress + 0.5));
    params["Progress"] = Util::toString(static_cast<int>(progress + 0.5)) + "%";
    params["Transferred"] = Util::formatBytes(s.sent);
    double speed = ul->getUserConnection().getDisplaySpeed();
    if(speed <= 0)
        speed = ul->getAverageSpeed();
    speed = TransfersDisplay::roundSpeed(speed);
    params["Speed"] = Util::toString(speed);
    int64_t timeLeft = -1;
    if (speed > 0 && s.fileSize > s.sent)
        timeLeft = static_cast<int64_t>((s.fileSize - s.sent) / speed);
    params["Time Left"] = Util::toString(timeLeft);
}

string uploadProgressStatus(int64_t sent, int64_t fileSize) {
    const int progress = fileSize > 0
        ? static_cast<int>(std::min(100.0, std::max(0.0, sent * 100.0 / fileSize)) + 0.5) : 0;
    return str(F_("Uploaded %1% (%2%%) ") % Util::formatBytes(sent) % progress);
}

} // namespace TransfersUpload
