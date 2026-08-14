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
#include "MainWindow.h"

QString FinishedTransferProxy::uploadTitle(){ return tr("Finished uploads"); }
QString FinishedTransferProxy::downloadTitle() { return tr("Finished downloads"); }

template <bool isUpload>
QString FinishedTransfers<isUpload>::getArenaShortTitle() {
    if (isUpload || newCount <= 0)
        return getArenaTitle();
    return downloadTitle() + QString(" (%1)").arg(newCount);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::noteNewDownload() {
    if (isUpload || isVisible())
        return;
    ++newCount;
    if (MainWindow *mw = MainWindow::getInstance())
        mw->redrawToolPanel();
}

template <bool isUpload>
void FinishedTransfers<isUpload>::clearNewDownloads() {
    if (isUpload || newCount <= 0)
        return;
    newCount = 0;
    if (MainWindow *mw = MainWindow::getInstance())
        mw->redrawToolPanel();
}

template QString FinishedTransfers<true>::getArenaShortTitle();
template QString FinishedTransfers<false>::getArenaShortTitle();
template void FinishedTransfers<true>::noteNewDownload();
template void FinishedTransfers<false>::noteNewDownload();
template void FinishedTransfers<true>::clearNewDownloads();
template void FinishedTransfers<false>::clearNewDownloads();

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
