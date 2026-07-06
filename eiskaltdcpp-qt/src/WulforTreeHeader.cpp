/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "WulforUtil.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QWidget>

static QHeaderView *viewHeader(QAbstractItemView *view)
{
    if (QTableView *table = qobject_cast<QTableView*>(view))
        return table->horizontalHeader();
    if (QTreeWidget *treeWidget = qobject_cast<QTreeWidget*>(view))
        return treeWidget->header();
    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        return tree->header();
    return nullptr;
}

static QList<int> visibleColumns(QHeaderView *header)
{
    QList<int> visible;
    for (int v = 0; v < header->count(); ++v) {
        const int i = header->logicalIndex(v);
        if (!header->isSectionHidden(i))
            visible.append(i);
    }
    return visible;
}

static int headerLabelWidth(QAbstractItemView *view, int column)
{
    QAbstractItemModel *model = view->model();
    if (!model)
        return 48;

    return qMax(48, QFontMetrics(view->font()).horizontalAdvance(
               model->headerData(column, Qt::Horizontal).toString()) + 24);
}

static void measureTreeRows(QAbstractItemModel *model, const QModelIndex &parent,
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

static int columnContentWidth(QAbstractItemView *view, QHeaderView *header, int column)
{
    int w = headerLabelWidth(view, column);

    if (QTreeView *tree = qobject_cast<QTreeView*>(view))
        tree->resizeColumnToContents(column);
    else if (QTableView *table = qobject_cast<QTableView*>(view))
        table->resizeColumnToContents(column);

    w = qMax(w, header->sectionSize(column));
    w = qMax(w, header->sectionSizeHint(column) + 16);

    QAbstractItemModel *model = view->model();
    if (!model)
        return w;

    const QFontMetrics fm(view->font());
    measureTreeRows(model, QModelIndex(), column, fm, 0, w);
    return w;
}

static void autosizeColumns(QAbstractItemView *view)
{
    QHeaderView *header = viewHeader(view);
    if (!view || !header || header->count() < 1)
        return;

    const int viewWidth = view->viewport()->width();
    if (viewWidth < 40)
        return;

    const QList<int> visible = visibleColumns(header);
    if (visible.isEmpty())
        return;

    header->setStretchLastSection(false);

    QVector<int> widths;
    widths.reserve(visible.size());
    int total = 0;
    for (int col : visible) {
        const int w = columnContentWidth(view, header, col);
        widths.append(w);
        total += w;
    }

    if (total < viewWidth - 8 && !widths.isEmpty())
        widths[0] += viewWidth - total;

    for (int i = 0; i < visible.size(); ++i)
        header->resizeSection(visible.at(i), widths.at(i));
}

static bool headerLayoutOk(QHeaderView *header, QAbstractItemView *view)
{
    const QList<int> visible = visibleColumns(header);
    if (visible.isEmpty())
        return true;

    if (!view->isVisible() || view->viewport()->width() < 40)
        return false;

    for (int col : visible) {
        if (header->sectionSize(col) < headerLabelWidth(view, col))
            return false;
    }

    int total = 0;
    for (int col : visible)
        total += header->sectionSize(col);

    return total >= view->viewport()->width() - 32;
}

static void checkTreeHeaderLayout(QAbstractItemView *view)
{
    if (!view)
        return;

    QHeaderView *header = viewHeader(view);
    if (!header)
        return;

    if (headerLayoutOk(header, view)) {
        view->setProperty("_eiskalt_autosized", true);
        return;
    }

    view->setProperty("_eiskalt_autosized", false);
    if (!view->isVisible() || view->viewport()->width() < 40)
        return;

    autosizeColumns(view);

    if (headerLayoutOk(header, view))
        view->setProperty("_eiskalt_autosized", true);
}

class TreeHeaderAutosizeFilter : public QObject
{
public:
    explicit TreeHeaderAutosizeFilter(QAbstractItemView *view) : QObject(view), m_view(view)
    {
        view->installEventFilter(this);
        if (view->viewport())
            view->viewport()->installEventFilter(this);

        for (QWidget *w = view->parentWidget(); w; w = w->parentWidget()) {
            w->installEventFilter(this);
            if (w->isWindow())
                break;
        }
    }

    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        Q_UNUSED(obj);
        if (!m_view || m_view->property("_eiskalt_autosized").toBool())
            return false;

        switch (ev->type()) {
        case QEvent::Show:
        case QEvent::Resize:
            checkTreeHeaderLayout(m_view);
            break;
        default:
            break;
        }
        return false;
    }

private:
    QPointer<QAbstractItemView> m_view;
};

static void watchTreeHeaderLayout(QAbstractItemView *view)
{
    if (!view)
        return;

    if (!view->property("_eiskalt_autosize_filter").toBool()) {
        view->setProperty("_eiskalt_autosize_filter", true);
        new TreeHeaderAutosizeFilter(view);
    }

    QAbstractItemModel *model = view->model();
    if (!model || view->property("_eiskalt_autosize_hook").toBool())
        return;

    view->setProperty("_eiskalt_autosize_hook", true);
    QObject::connect(model, &QAbstractItemModel::rowsInserted, view, [view]() {
        view->setProperty("_eiskalt_autosized", false);
        checkTreeHeaderLayout(view);
    });
    QObject::connect(model, &QAbstractItemModel::modelReset, view, [view]() {
        view->setProperty("_eiskalt_autosized", false);
        checkTreeHeaderLayout(view);
    });
    QObject::connect(model, &QAbstractItemModel::columnsInserted, view, [view]() {
        view->setProperty("_eiskalt_autosized", false);
        checkTreeHeaderLayout(view);
    });
}

void WulforUtil::restoreTreeHeader(QHeaderView *header, const QByteArray &state)
{
    if (!header)
        return;

    QAbstractItemView *view = qobject_cast<QAbstractItemView*>(header->parentWidget());
    if (!view)
        return;

    header->setStretchLastSection(false);
    view->setProperty("_eiskalt_autosized", false);

    if (!state.isEmpty())
        header->restoreState(state);

    watchTreeHeaderLayout(view);
    checkTreeHeaderLayout(view);
}

void WulforUtil::ensureTreeHeaderAutosized(QAbstractItemView *view)
{
    if (!view)
        return;

    view->setProperty("_eiskalt_autosized", false);
    watchTreeHeaderLayout(view);
    checkTreeHeaderLayout(view);
}
