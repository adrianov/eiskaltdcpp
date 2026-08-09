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

#include "treeheader/TreeHeaderAutosize.h"
#include "treeheader/HeaderColumnFit.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QHeaderView>
#include <QMetaObject>

namespace {
const char kObjectName[] = "TreeHeaderAutosize";
}

TreeHeaderAutosize *TreeHeaderAutosize::attached(QAbstractItemView *view)
{
    if (!view)
        return nullptr;
    if (QObject *existing = view->findChild<QObject*>(
            QLatin1String(kObjectName), Qt::FindDirectChildrenOnly))
        return static_cast<TreeHeaderAutosize*>(existing);
    return new TreeHeaderAutosize(view);
}

TreeHeaderAutosize::TreeHeaderAutosize(QAbstractItemView *view)
    : QObject(view)
    , view_(view)
{
    setObjectName(QLatin1String(kObjectName));
    // View only — ancestor Resize (MainWindow / side dock) must not fit columns.
    view->installEventFilter(this);
    hookModel();
}

void TreeHeaderAutosize::setStretchColumn(QAbstractItemView *view, int logicalColumn)
{
    TreeHeaderAutosize *a = attached(view);
    if (!a || a->stretchColumn_ == logicalColumn)
        return;
    a->stretchColumn_ = logicalColumn;
    a->rowsSized_ = false;
    a->requestFit();
}

void TreeHeaderAutosize::restore(QHeaderView *header, const QByteArray &state)
{
    if (!header)
        return;
    QAbstractItemView *view = qobject_cast<QAbstractItemView*>(header->parentWidget());
    if (!view)
        return;
    header->setStretchLastSection(false);
    if (!state.isEmpty())
        header->restoreState(state);
    TreeHeaderAutosize *a = attached(view);
    a->rowsSized_ = false;
    a->requestFit();
}

void TreeHeaderAutosize::ensure(QAbstractItemView *view)
{
    if (!view)
        return;
    TreeHeaderAutosize *a = attached(view);
    a->rowsSized_ = false;
    a->requestFit();
}

void TreeHeaderAutosize::requestFit()
{
    done_ = false;
    hookModel();
    scheduleCheck();
}

void TreeHeaderAutosize::hookModel()
{
    if (!view_ || modelHooked_)
        return;
    QAbstractItemModel *model = view_->model();
    if (!model)
        return;
    modelHooked_ = true;
    connect(model, &QAbstractItemModel::rowsInserted, this, [this]() { requestFit(); });
    connect(model, &QAbstractItemModel::columnsInserted, this, [this]() { requestFit(); });
    connect(model, &QAbstractItemModel::modelReset, this, [this]() {
        rowsSized_ = false;
        requestFit();
    });
}

void TreeHeaderAutosize::scheduleCheck()
{
    if (pending_)
        return;
    pending_ = true;
    QMetaObject::invokeMethod(this, [this]() {
        pending_ = false;
        checkLayout();
    }, Qt::QueuedConnection);
}

bool TreeHeaderAutosize::eventFilter(QObject *obj, QEvent *ev)
{
    Q_UNUSED(obj);
    if (!view_ || done_)
        return false;
    // Resize only until the first successful fit — later dock drags stay free.
    if (ev->type() == QEvent::Show || ev->type() == QEvent::Resize)
        scheduleCheck();
    return false;
}

void TreeHeaderAutosize::checkLayout()
{
    if (!view_)
        return;
    HeaderColumnFit fit(view_, stretchColumn_);
    const bool hasRows = view_->model() && view_->model()->rowCount() > 0;
    // Viewport fill alone is not enough before the first content fit — empty
    // fits leave header-sized columns that stay narrow after rows arrive.
    if (fit.isAdequate() && (!hasRows || rowsSized_)) {
        done_ = true;
        return;
    }
    done_ = false;
    if (!fit.canApply())
        return;
    fit.apply();
    done_ = fit.isAdequate();
    if (hasRows && done_)
        rowsSized_ = true;
}
