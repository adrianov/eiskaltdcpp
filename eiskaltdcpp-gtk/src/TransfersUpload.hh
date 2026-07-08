/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <dcpp/stdinc.h>
#include <dcpp/Upload.h>
#include <dcpp/StringMap.h>

namespace TransfersUpload {

struct UploadUiState {
    int64_t fileSize = 0;
    int64_t sent = 0;
    bool continuing = false;
    bool fileDone = false;
};

UploadUiState uploadState(const dcpp::Upload *ul);
std::string uploadTickKey(const dcpp::Upload *ul);
bool shouldRefreshUploadUi(const std::string& key);
void clearUploadUiThrottle(const std::string& key);
void setUploadParams(dcpp::StringMap& params, dcpp::Upload* ul, const UploadUiState& s);
std::string uploadProgressStatus(int64_t sent, int64_t fileSize);

} // namespace TransfersUpload
