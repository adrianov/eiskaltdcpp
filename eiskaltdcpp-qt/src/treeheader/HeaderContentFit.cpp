/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "treeheader/HeaderContentFit.h"
#include "treeheader/ColumnContentSpan.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QVector>

namespace {

struct ColNeed {
    int col = 0;
    int need = 0;
};

} // namespace

HeaderContentFit::HeaderContentFit(QAbstractItemView *view, const QSet<int> &manual)
    : view_(view)
    , manual_(manual)
{
}

QHeaderView *HeaderContentFit::headerOf(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *tw = qobject_cast<QTreeWidget*>(view))
        return tw->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
}

bool HeaderContentFit::ready() const
{
    return view_ && view_->isVisible() && view_->viewport()
        && view_->viewport()->width() >= 40 && headerOf(view_);
}

void HeaderContentFit::fit(bool grow)
{
    QHeaderView *header = headerOf(view_);
    if (!header || !view_->viewport() || header->count() < 1)
        return;
    header->setStretchLastSection(false);

    ColumnContentSpan span(view_);
    QVector<ColNeed> freeCols;
    int totalNeed = 0;
    bool anySlack = false;
    bool needsGrow = false;
    for (int v = 0; v < header->count(); ++v) {
        const int col = header->logicalIndex(v);
        if (header->isSectionHidden(col))
            continue;
        if (manual_.contains(col)) {
            totalNeed += header->sectionSize(col);
            continue;
        }
        const int need = span.cells(col);
        int have = header->sectionSize(col);
        if (grow && have < need) {
            header->resizeSection(col, need);
            have = need;
        }
        if (have < need)
            needsGrow = true;
        if (have > need)
            anySlack = true;
        totalNeed += need;
        freeCols.append(ColNeed{col, need});
    }

    // Shrink only with horizontal scroll, no pending grow, and content fits.
    const int viewW = view_->viewport()->width();
    if (needsGrow || !anySlack || header->length() <= viewW || totalNeed > viewW)
        return;
    for (const ColNeed &item : freeCols)
        header->resizeSection(item.col, item.need);
}

void HeaderContentFit::apply()
{
    fit(true);
}

void HeaderContentFit::shrinkSlack()
{
    fit(false);
}
