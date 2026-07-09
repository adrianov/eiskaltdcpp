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
#include "WulforUtil.h"
#include "WulforSettings.h"

#include <QMenu>

void SearchFrame::rememberSearch(const QString &s)
{
    Q_D(SearchFrame);

    if (d->searchHistory.contains(s))
        d->searchHistory.removeAt(d->searchHistory.indexOf(s));

    const bool isTTH = WulforUtil::isTTH(s);

    if ((WBGET("memorize-tth-search-phrases", false) && isTTH) || !isTTH)
        d->searchHistory.push_front(s);

    QMenu *m = new QMenu();
    for (const auto &entry : d->searchHistory)
        m->addAction(entry);

    lineEdit_SEARCHSTR->setMenu(m);

    const int maxItemsNumber = WIGET("search-history-items-number", 10);
    while (d->searchHistory.count() > maxItemsNumber)
        d->searchHistory.removeLast();

    const QString hist = d->searchHistory.join("\n");
    WSSET(WS_SEARCH_HISTORY, hist.toUtf8().toBase64());
}
