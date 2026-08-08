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
#include "MediaEnrichQueue.h"

#include <QHeaderView>
#include <QHash>
#include <QVariant>
#include <QVariantMap>

void SearchFrame::applyMediaEnrich(const QVariant &packed)
{
    Q_D(SearchFrame);
    if (!d->model)
        return;

    const QVariantMap outer = packed.toMap();
    QHash<QString, QVariantMap> media;
    for (auto it = outer.constBegin(); it != outer.constEnd(); ++it)
        media.insert(it.key(), it.value().toMap());
    if (media.isEmpty())
        return;

    d->model->applyMediaByTth(media);
    applyOptionalColumns();
}

void SearchFrame::applyOptionalColumns()
{
    Q_D(SearchFrame);
    if (!d->model || !treeView_RESULTS)
        return;
    QHeaderView *h = treeView_RESULTS->header();
    h->setSectionHidden(COLUMN_SF_BR, !d->model->hasBitrate());
    h->setSectionHidden(COLUMN_SF_WH, !d->model->hasResolution());
    h->setSectionHidden(COLUMN_SF_MVIDEO, !d->model->hasVideo());
    h->setSectionHidden(COLUMN_SF_MAUDIO, !d->model->hasAudio());
}

void SearchFrame::requeueMissingMedia()
{
    Q_D(SearchFrame);
    if (!d->model || !d->mediaEnrich)
        return;
    d->mediaEnrich->queue(d->model->missingMediaTths());
}
