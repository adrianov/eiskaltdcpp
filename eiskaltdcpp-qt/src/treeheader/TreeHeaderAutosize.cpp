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

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>

namespace {
const char kObjectName[] = "TreeHeaderAutosize";

void measureTreeRows(QAbstractItemModel *model, const QModelIndex &parent,
                     int column, const QFontMetrics &fm, int depth, int &w)
{
    if (!model || depth > 24)
        return;

    const int rows = qMin(model->rowCount(parent), 300);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, column, parent);
        if (!idx.isValid())
            continue;

        int rowW = fm.horizontalAdvance(idx.data(Qt::DisplayRole).toString()) + 20;
        const QVariant deco = idx.data(Qt::DecorationRole);
        if (deco.canConvert<QPixmap>())
            rowW += deco.value<QPixmap>().width() + 4;
        else if (deco.canConvert<QIcon>()) {
            const QIcon icon = deco.value<QIcon>();
            if (!icon.isNull())
                rowW += icon.actualSize(QSize(16, 16)).width() + 4;
        }
        w = qMax(w, rowW);

        if (model->hasChildren(idx))
            measureTreeRows(model, idx, column, fm, depth + 1, w);
    }
}
} // namespace

QHeaderView *TreeHeaderAutosize::viewHeader(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *treeWidget = qobject_cast<QTreeWidget*>(view))
        return treeWidget->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
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
    view->installEventFilter(this);
    if (view->viewport())
        view->viewport()->installEventFilter(this);
    for (QWidget *w = view->parentWidget(); w; w = w->parentWidget()) {
        w->installEventFilter(this);
        if (w->isWindow())
            break;
    }
    hookModel();
}

void TreeHeaderAutosize::setStretchColumn(QAbstractItemView *view, int logicalColumn)
{
    if (TreeHeaderAutosize *a = attached(view))
        a->stretchColumn_ = logicalColumn;
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
    a->done_ = false;
    a->hookModel();
    a->checkLayout();
}

void TreeHeaderAutosize::ensure(QAbstractItemView *view)
{
    if (!view)
        return;
    TreeHeaderAutosize *a = attached(view);
    a->done_ = false;
    a->hookModel();
    a->checkLayout();
}

void TreeHeaderAutosize::hookModel()
{
    if (!view_ || modelHooked_)
        return;
    QAbstractItemModel *model = view_->model();
    if (!model)
        return;

    modelHooked_ = true;
    const auto dirty = [this]() {
        done_ = false;
        checkLayout();
    };
    connect(model, &QAbstractItemModel::rowsInserted, this, dirty);
    connect(model, &QAbstractItemModel::modelReset, this, dirty);
    connect(model, &QAbstractItemModel::columnsInserted, this, dirty);
}

bool TreeHeaderAutosize::eventFilter(QObject *obj, QEvent *ev)
{
    Q_UNUSED(obj);
    if (!view_ || done_)
        return false;
    if (ev->type() == QEvent::Show || ev->type() == QEvent::Resize)
        checkLayout();
    return false;
}

QList<int> TreeHeaderAutosize::visibleColumns() const
{
    QList<int> visible;
    QHeaderView *header = viewHeader(view_);
    if (!header)
        return visible;
    for (int v = 0; v < header->count(); ++v) {
        const int i = header->logicalIndex(v);
        if (!header->isSectionHidden(i))
            visible.append(i);
    }
    return visible;
}

int TreeHeaderAutosize::headerLabelWidth(int column) const
{
    QAbstractItemModel *model = view_->model();
    if (!model)
        return 48;
    return qMax(48, QFontMetrics(view_->font()).horizontalAdvance(
                   model->headerData(column, Qt::Horizontal).toString()) + 24);
}

int TreeHeaderAutosize::columnContentWidth(int column) const
{
    QHeaderView *header = viewHeader(view_);
    int w = headerLabelWidth(column);

    if (QTreeView *tree = qobject_cast<QTreeView*>(view_))
        tree->resizeColumnToContents(column);
    else if (QTableView *table = qobject_cast<QTableView*>(view_))
        table->resizeColumnToContents(column);

    w = qMax(w, header->sectionSize(column));
    w = qMax(w, header->sectionSizeHint(column) + 16);

    QAbstractItemModel *model = view_->model();
    if (!model)
        return w;

    measureTreeRows(model, QModelIndex(), column, QFontMetrics(view_->font()), 0, w);
    return w;
}

int TreeHeaderAutosize::stretchIndex(const QList<int> &visible) const
{
    if (visible.isEmpty())
        return 0;
    if (stretchColumn_ >= 0) {
        const int idx = visible.indexOf(stretchColumn_);
        if (idx >= 0)
            return idx;
    }
    return 0;
}

bool TreeHeaderAutosize::layoutOk() const
{
    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return true;
    if (!view_->isVisible() || view_->viewport()->width() < 40)
        return false;

    QHeaderView *header = viewHeader(view_);
    for (int col : visible) {
        if (header->sectionSize(col) < headerLabelWidth(col))
            return false;
    }

    int total = 0;
    for (int col : visible)
        total += header->sectionSize(col);
    return total >= view_->viewport()->width() - 32;
}

void TreeHeaderAutosize::autosizeColumns()
{
    QHeaderView *header = viewHeader(view_);
    if (!header || header->count() < 1)
        return;

    const int viewWidth = view_->viewport()->width();
    if (viewWidth < 40)
        return;

    const QList<int> visible = visibleColumns();
    if (visible.isEmpty())
        return;

    header->setStretchLastSection(false);

    QVector<int> widths;
    widths.reserve(visible.size());
    int total = 0;
    for (int col : visible) {
        const int w = columnContentWidth(col);
        widths.append(w);
        total += w;
    }

    const int stretch = stretchIndex(visible);
    if (total < viewWidth - 8) {
        widths[stretch] += viewWidth - total;
    } else if (total > viewWidth && stretchColumn_ >= 0) {
        const int minW = headerLabelWidth(visible.at(stretch));
        const int shrink = qMin(total - viewWidth, qMax(0, widths[stretch] - minW));
        if (shrink > 0)
            widths[stretch] -= shrink;
    }

    for (int i = 0; i < visible.size(); ++i)
        header->resizeSection(visible.at(i), widths.at(i));
}

void TreeHeaderAutosize::checkLayout()
{
    if (!view_ || !viewHeader(view_))
        return;

    if (layoutOk()) {
        done_ = true;
        return;
    }

    done_ = false;
    if (!view_->isVisible() || view_->viewport()->width() < 40)
        return;

    autosizeColumns();
    if (layoutOk())
        done_ = true;
}
