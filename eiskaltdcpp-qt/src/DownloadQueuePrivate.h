/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QMap>
#include <QHash>
#include <QShortcut>

#include "DownloadQueueModel.h"

class DownloadQueuePrivate {
    typedef QMap<QString, QMap<QString, QString> > SourceMap;

public:
    QShortcut *deleteShortcut = nullptr;

    DownloadQueueModel *queue_model = nullptr;
    DownloadQueueModel *file_model = nullptr;
    DownloadQueueDelegate *delegate = nullptr;

    DownloadQueue::Menu *menu = nullptr;

    SourceMap sources;
    SourceMap badSources;
    /** Last auto index-match per target (ms); avoids CTM storms when holders stay unmatched. */
    QHash<QString, qint64> lastIndexAttach;

    QList<QVariantMap> pendingAdds;
    bool flushAddsQueued = false;
};
