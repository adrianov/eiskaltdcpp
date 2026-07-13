/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrame.h"
#include "SearchFramePrivate.h"
#include "SearchModel.h"
#include "WulforUtil.h"

#include "dcpp/QueueItem.h"

using namespace dcpp;

static void emitQueuedTth(SearchFrame *frame, QueueItem *qi)
{
    if (!qi || qi->isSet(QueueItem::FLAG_USER_LIST))
        return;
    // QueuedConnection slot clears local/queue caches for this TTH.
    frame->coreDownloadFinished(_q(qi->getTTH().toBase32()));
}

void SearchFrame::on(QueueManagerListener::Added, QueueItem *qi) noexcept {
    emitQueuedTth(this, qi);
}

void SearchFrame::on(QueueManagerListener::Removed, QueueItem *qi) noexcept {
    emitQueuedTth(this, qi);
}

void SearchFrame::on(QueueManagerListener::Finished, QueueItem *qi, const string&, int64_t) noexcept {
    emitQueuedTth(this, qi);
}

void SearchFrame::on(QueueManagerListener::FileMoved, const string&) noexcept {
    emit coreDownloadFinished(QString());
}

void SearchFrame::slotDownloadFinished(const QString &tth){
    Q_D(SearchFrame);

    if (!d->model)
        return;

    if (tth.isEmpty()) {
        // FileMoved has no TTH; coalesce already-queued empties into one refresh.
        if (d->localRefreshPending)
            return;
        d->localRefreshPending = true;
        QMetaObject::invokeMethod(this, "slotLocalRefreshAll", Qt::QueuedConnection);
        return;
    }

    d->model->refreshLocal(tth);
}

void SearchFrame::slotLocalRefreshAll(){
    Q_D(SearchFrame);

    d->localRefreshPending = false;
    if (d->model)
        d->model->refreshLocal(QString());
}
