/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferViewPath.h"
#include "TransferView.h"
#include "TransferViewModel.h"
#include "WulforUtil.h"

#include "dcpp/File.h"
#include "dcpp/QueueManager.h"
#include "dcpp/QueueItem.h"
#include "dcpp/ShareManager.h"
#include "dcpp/Util.h"

namespace TransferViewPath {

bool isOpenableTransfer(const QString &filename) {
    return !filename.isEmpty()
        && filename != TransferView::tr("File list")
        && !filename.startsWith(QStringLiteral("TTH: "));
}

QString resolveUploadPath(const QString &target) {
    const std::string pathStr = _tq(target);
    if (!pathStr.empty() && dcpp::File::getSize(pathStr) > -1)
        return target;

    try {
        const auto paths = dcpp::ShareManager::getInstance()->getRealPaths(dcpp::Util::toAdcFile(pathStr));
        if (!paths.empty() && dcpp::File::getSize(paths.front()) > -1)
            return _q(paths.front());
    } catch (...) {}
    return QString();
}

QString resolveDownloadPath(const QString &target) {
    const std::string pathStr = _tq(target);
    dcpp::QueueManager *qm = dcpp::QueueManager::getInstance();
    std::string tempPath;
    std::string finalPath;

    {
        const dcpp::QueueItem::StringMap &ll = qm->lockQueue();
        const auto it = ll.find(const_cast<std::string*>(&pathStr));
        if (it != ll.end()) {
            tempPath = it->second->getTempTarget();
            finalPath = it->second->getTarget();
        }
        qm->unlockQueue();
    }

    if (!tempPath.empty() && dcpp::File::getSize(tempPath) > -1)
        return _q(tempPath);
    if (!finalPath.empty() && dcpp::File::getSize(finalPath) > -1)
        return _q(finalPath);
    if (dcpp::File::getSize(pathStr) > -1)
        return target;
    return QString();
}

QString resolveTransferPath(const TransferViewItem *item) {
    if (!item || item->target.isEmpty() || !isOpenableTransfer(item->data(COLUMN_TRANSFER_FNAME).toString()))
        return QString();
    return item->download ? resolveDownloadPath(item->target) : resolveUploadPath(item->target);
}

} // namespace TransferViewPath
