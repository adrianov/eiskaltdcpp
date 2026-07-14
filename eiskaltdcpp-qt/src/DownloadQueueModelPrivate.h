/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "DownloadQueueModel.h"

#include <QSet>

class DownloadQueueModelPrivate {
public:
    quint64 total_files = 0;
    quint64 total_size = 0;
    QSet<DownloadQueueItem*> batchExpand;
    int sortColumn = COLUMN_DOWNLOADQUEUE_NAME;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    DownloadQueueItem *rootItem = nullptr;
};
