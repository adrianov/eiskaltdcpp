/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "DownloadQueue.h"
#include "DownloadQueuePrivate.h"
#include "DownloadQueueModel.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/User.h"
#include "dcpp/QueueManager.h"
#include "dcpp/Exception.h"

using namespace dcpp;

void DownloadQueue::on(QueueManagerListener::Added, QueueItem *item) noexcept{
    VarMap params;
    getParams(params, item);

    emit coreAdded(params);
    emit added(_q(item->getTargetFileName()));
}

void DownloadQueue::on(QueueManagerListener::Moved, QueueItem *item, const std::string &oldTarget) noexcept{
    VarMap params;
    getParams(params, item);

    emit coreMoved(params);
    emit moved(_q(oldTarget), _q(item->getTargetFileName()));
}

void DownloadQueue::on(QueueManagerListener::Removed, QueueItem *item) noexcept{
    VarMap params;
    getParams(params, item);

    emit coreRemoved(params);
    emit removed(_q(item->getTargetFileName()));
}

void DownloadQueue::on(QueueManagerListener::SourcesUpdated, QueueItem *item) noexcept{
    VarMap params;
    getParams(params, item);

    emit coreSourcesUpdated(params);
}

void DownloadQueue::on(QueueManagerListener::StatusUpdated, QueueItem *item) noexcept{
    VarMap params;
    getParams(params, item);

    emit coreStatusUpdated(params);
}
