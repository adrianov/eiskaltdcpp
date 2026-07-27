/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"
#include "FinishedTransfersProxy.h"

QString FinishedTransferProxy::uploadTitle(){ return tr("Finished uploads"); }
QString FinishedTransferProxy::downloadTitle() { return tr("Finished downloads"); }

template <bool isUpload>
bool FinishedTransfers<isUpload>::isFileListPath(const std::string &file) {
    return isFinishedFileList(file);
}

template <bool isUpload>
bool FinishedTransfers<isUpload>::showDownload(const std::string &file, const FinishedFileItemPtr &item) const {
    if (isUpload)
        return true;
    return item->isFull() && !isFileListPath(file);
}

template <bool isUpload>
bool FinishedTransfers<isUpload>::showDownloadParams(const VarMap &params) const {
    if (isUpload)
        return true;
    if (!params["FULL"].toBool())
        return false;

    const string target = _tq(params["TARGET"].toString());
    const string path = _tq(params["PATH"].toString() + params["FNAME"].toString());
    return !isFileListPath(target) && !isFileListPath(path);
}

template bool FinishedTransfers<true>::isFileListPath(const std::string&);
template bool FinishedTransfers<false>::isFileListPath(const std::string&);
template bool FinishedTransfers<true>::showDownload(const std::string&, const FinishedFileItemPtr&) const;
template bool FinishedTransfers<false>::showDownload(const std::string&, const FinishedFileItemPtr&) const;
template bool FinishedTransfers<true>::showDownloadParams(const QVariantMap&) const;
template bool FinishedTransfers<false>::showDownloadParams(const QVariantMap&) const;
