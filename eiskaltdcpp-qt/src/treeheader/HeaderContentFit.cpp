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
#include "treeheader/HeaderWidthPlan.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>

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

void HeaderContentFit::apply()
{
    QHeaderView *header = headerOf(view_);
    if (!header || !view_->viewport() || header->count() < 1)
        return;
    header->setStretchLastSection(false);

    HeaderWidthPlan plan(view_, manual_);
    if (!plan.measure(header))
        return;

    const int viewW = view_->viewport()->width();
    if (plan.sumFull() <= viewW) {
        plan.setFull(header);
        plan.fillSpare(header, viewW - plan.sumFull());
        return;
    }

    plan.setSoft(header);
    if (plan.sumSoft() < viewW)
        plan.fillSpare(header, viewW - plan.sumSoft());
    else if (plan.sumSoft() > viewW)
        plan.shrinkOverflow(header, viewW);
}
