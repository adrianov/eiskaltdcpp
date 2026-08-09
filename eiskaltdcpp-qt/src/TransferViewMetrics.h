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
    int64_t fileSize = 0;   /**< Full file (Size column and peer %). */
    int64_t segmentPos = 0; /**< In-flight segment bytes (SEGP). */
    bool continuing = false;
};

int64_t downloadFileSize(const dcpp::Transfer *trf);

UploadUiState uploadState(const dcpp::Upload *ul);
DownloadUiState downloadState(const dcpp::Download *dl);

QString uploadProgressStat(int64_t sent, int64_t fileSize);
/** Status text for bytes/size (file group or active segment). */
QString downloadProgressStat(int64_t bytes, int64_t size);
/** 1-based remote upload queue position; 0 if unknown. */
QString slotWaitStat(qint64 queuePos);

void applyUploadMetrics(QVariantMap &params, const UploadUiState &s, const QString &stat);
void applyDownloadMetrics(QVariantMap &params, const DownloadUiState &s, const QString &stat);
void applyUploadSpeed(QVariantMap &params, const dcpp::Upload *ul, const UploadUiState &s);
void applyDownloadSpeed(QVariantMap &params, const dcpp::Download *dl, const DownloadUiState &s);

QString uploadTickKey(const dcpp::Upload *ul);
QString downloadTickKey(const dcpp::Download *dl);

bool shouldRefreshUploadUi(const QString &key);
bool shouldRefreshDownloadUi(const QString &key);
void clearUploadUiThrottle(const QString &key);
void clearDownloadUiThrottle(const QString &key);
void clearUploadUiThrottleByCid(const QString &cid);
void clearDownloadUiThrottleByCid(const QString &cid);

} // namespace TransferViewMetrics
