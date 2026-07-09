/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QVariantMap>
#include <QString>
#include <cstdint>

namespace dcpp {
class Download;
class Transfer;
class Upload;
}

namespace TransferViewMetrics {

struct UploadUiState {
    int64_t fileSize = 0;
    int64_t sent = 0;
    bool continuing = false;
    bool fileDone = false;
};

struct DownloadUiState {
    int64_t fileSize = 0;
    int64_t downloaded = 0;
    bool continuing = false;
};

int64_t downloadFileSize(const dcpp::Transfer *trf);

UploadUiState uploadState(const dcpp::Upload *ul);
DownloadUiState downloadState(const dcpp::Download *dl);

QString uploadProgressStat(int64_t sent, int64_t fileSize);
QString downloadProgressStat(int64_t downloaded, int64_t fileSize);
/** 1-based remote upload queue position; 0 if unknown. trailingSpace for parent status suffix. */
QString slotWaitStat(qint64 queuePos, bool trailingSpace = false);

void applyUploadMetrics(QVariantMap &params, const UploadUiState &s, const QString &stat);
void applyDownloadMetrics(QVariantMap &params, const dcpp::Download *dl,
                          const DownloadUiState &s, const QString &stat);
void applyUploadSpeed(QVariantMap &params, const dcpp::Upload *ul, const UploadUiState &s);
void applyDownloadSpeed(QVariantMap &params, const dcpp::Download *dl, const DownloadUiState &s);

QString downloadTickKey(const dcpp::Download *dl);
bool shouldRefreshDownloadUi(const QString &key);
void clearDownloadUiThrottle(const QString &key);
void clearDownloadUiThrottleByCid(const QString &cid);

} // namespace TransferViewMetrics
