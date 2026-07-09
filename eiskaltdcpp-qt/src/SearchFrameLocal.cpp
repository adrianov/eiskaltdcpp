/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchFrameLocal.h"
#include "SearchFrame.h"
#include "ShareIndex.h"
#include "ShareBrowser.h"

#include <QMetaObject>

namespace SearchFrameLocal {

void startLocalSearch(SearchFrame *frame, const QStringList &terms, bool isHash,
                      bool dirsOnly, bool filesOnly, qint64 size, int sizeMode,
                      const QStringList &extensions)
{
    if (!frame || terms.isEmpty())
        return;

    ShareIndex::SearchFilter filter;
    filter.terms = terms;
    filter.extensions = extensions;
    filter.isHash = isHash;
    filter.dirsOnly = dirsOnly;
    filter.filesOnly = filesOnly;
    filter.size = size;
    filter.sizeMode = sizeMode;
    filter.limit = 500;

    AsyncRunner *runner = new AsyncRunner(frame);
    runner->setRunFunction([frame, filter]() {
        const QList<QVariantMap> rows = ShareIndex::getInstance()->search(filter);
        for (const QVariantMap &map : rows) {
            QMetaObject::invokeMethod(frame, "addResult", Qt::QueuedConnection,
                                      Q_ARG(QVariantMap, map));
        }
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

void upsertHubResult(const QVariantMap &map)
{
    // Keep SearchManager SR path off the SQLite lock.
    AsyncRunner *runner = new AsyncRunner();
    runner->setRunFunction([map]() {
        ShareIndex::getInstance()->upsertFromSearch(map);
    });
    QObject::connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->start();
}

} // namespace SearchFrameLocal
