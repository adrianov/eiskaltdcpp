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
#include "SearchFrameLocal.h"
#include "WulforUtil.h"
#include "GlobalTimer.h"

using namespace dcpp;

void SearchFrame::slotTimer(){
    Q_D(SearchFrame);

#ifdef USE_QT_SQLITE
    if (++d->indexStatsTick >= 30) {
        d->indexStatsTick = 0;
        SearchFrameLocal::refreshIndexStats(this);
    }
#endif

    if (d->waitingResults) {
        uint64_t now = GlobalTimer::getInstance()->getTicks()*1000;
        float fraction  = 100.0f*(now - d->searchStartTime)/(d->searchEndTime - d->searchStartTime);
        if (fraction >= 100.0) {
            fraction = 100.0;
            d->waitingResults = false;
        }
        const QString msg = tr("Searching for %1 ...").arg(d->target);
        progressBar->setFormat(msg);
        progressBar->setValue(static_cast<unsigned>(fraction));
    }
    else {
        progressBar->setFormat(QString());
        progressBar->setValue(0);
    }

    if (d->dropped == d->results && !d->dropped && !d->filtered){

        if (d->currentSearch.empty())
            frame_PROGRESS->hide();
        else {
            frame_PROGRESS->show();
            const QString text = tr("<b>No results</b>");
            if (status->text() != text)
                status->setText(text);
        }
    }
    else {
        if (!frame_PROGRESS->isVisible())
            frame_PROGRESS->show();

        QString text = QString(tr("Found: <b>%1</b>  Dropped: <b>%2</b>"))
                .arg(d->results).arg(d->dropped);
        if (d->filtered)
            text += QString(tr("  Filtered: <b>%1</b>")).arg(d->filtered);

        if (status->text() != text)
            status->setText(text);
    }
}

void SearchFrame::setIndexStats(const QString &text)
{
#ifdef USE_QT_SQLITE
    label_INDEX_STATS->setText(text);
#else
    Q_UNUSED(text);
#endif
}
