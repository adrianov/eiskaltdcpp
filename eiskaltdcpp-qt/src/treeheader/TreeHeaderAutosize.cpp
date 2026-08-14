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
#include "treeheader/ColumnPeakWatch.h"
#include "treeheader/HeaderContentFit.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QHeaderView>
#include <QTimer>

namespace {
const char kObjectName[] = "TreeHeaderAutosize";
constexpr int kDebounceMs = 200;
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
    , debounce_(new QTimer(this))
    , peaks_(std::make_unique<ColumnPeakWatch>(view))
{
    setObjectName(QLatin1String(kObjectName));
    debounce_->setSingleShot(true);
    debounce_->setInterval(kDebounceMs);
    connect(debounce_, &QTimer::timeout, this, [this]() { checkLayout(); });
    peaks_->setManual(&manual_);
    peaks_->setNeedFit([this]() { requestFit(); });
    view->installEventFilter(this);
    hookHeader();
    hookModel();
}

void TreeHeaderAutosize::hookHeader()
{
    QHeaderView *header = HeaderContentFit::headerOf(view_);
    if (!header)
        return;
    connect(header, &QHeaderView::sectionResized, this, [this](int logical, int, int) {
        if (!fitting_)
            manual_.insert(logical);
    });
}

void TreeHeaderAutosize::restore(QHeaderView *header, const QByteArray &state)
{
    if (!header)
        return;
    QAbstractItemView *view = qobject_cast<QAbstractItemView*>(header->parentWidget());
    if (!view)
        return;
    TreeHeaderAutosize *a = attached(view);
    a->fitting_ = true;
    if (!state.isEmpty())
        header->restoreState(state);
    header->setStretchLastSection(false);
    a->fitting_ = false;
    a->manual_.clear();
    a->peaks_->clearPeaks();
    a->hookModel();
    a->requestFit();
}

void TreeHeaderAutosize::ensure(QAbstractItemView *view)
{
    if (!view)
        return;
    attached(view)->requestFit();
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

    connect(model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex &parent, int first, int last) {
                peaks_->onInserted(parent, first, last);
            });
    connect(model, &QAbstractItemModel::columnsInserted, this, [this]() {
        requestFit();
    });
    connect(model, &QAbstractItemModel::modelReset, this, [this]() {
        peaks_->onReset();
    });
    connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
        peaks_->onReset();
    });
    connect(model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles) {
                peaks_->onDataChanged(tl, br, roles);
            });
    // layoutChanged (sort) is not connected: values do not change.
}

void TreeHeaderAutosize::scheduleCheck()
{
    debounce_->start();
}

bool TreeHeaderAutosize::eventFilter(QObject *obj, QEvent *ev)
{
    Q_UNUSED(obj);
    if (!view_)
        return false;
    if (ev->type() == QEvent::Show) {
        if (!done_)
            scheduleCheck();
    } else if (ev->type() == QEvent::Resize) {
        scheduleCheck();
    }
    return false;
}

void TreeHeaderAutosize::checkLayout()
{
    HeaderContentFit fit(view_, manual_);
    if (!fit.ready())
        return;
    fitting_ = true;
    peaks_->recheck();
    peaks_->mergePeaks(fit.apply(peaks_->peaks()));
    fitting_ = false;
    done_ = true;
}
