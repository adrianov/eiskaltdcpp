/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "FileBrowserModel.h"

#include <QFuture>

#include <QtConcurrent>

using namespace dcpp;

void ShareBrowser::slotRightPaneSelChanged(const QItemSelection &, const QItemSelection &){
    QModelIndexList list        = treeView_RPANE->selectionModel()->selectedRows(COLUMN_FILEBROWSER_NAME);
    qulonglong selected_size    = 0;
    quint32    total_selected   = 0;

    for (const auto &i : list) {
        const QModelIndex src = proxy ? proxy->mapToSource(i) : i;
        FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(src.internalPointer());
        if (!item)
            continue;
        selected_size += item->data(COLUMN_FILEBROWSER_ESIZE).toULongLong();
        total_selected++;
    }

    QString status;
    const int shown = proxy ? proxy->rowCount() : list_root->childCount();

    if (total_selected > 0)
        status = tr("Selected %1 from %2 items; ").arg(total_selected).arg(shown);

    status += totalStatusText();

    if (selected_size > 0)
        status += tr("; Selected: %1").arg(WulforUtil::formatBytes(selected_size));

    label_RIGHT->setText(status);
}

static bool onlyFirstColumn(const QModelIndex &index){
    return (index.column() == 0);
}

void ShareBrowser::slotLeftPaneSelChanged(const QItemSelection &sel, const QItemSelection &des){
    Q_UNUSED(sel)

    if (flatMode)
        return;

    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    // Multi-select (Shift/Ctrl) keeps the current listing; browse only on a single selection.
    if (selected.size() != 1)
        return;

    QModelIndex index = treeMapToSource(selected.at(0));
    if (!index.isValid())
        return;

    SelPair p;
    FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());

    changeRoot(item->dir);
    p.dir = item->dir;
    p.index = index;
    p.path_tesxt = tree_model->createRemotePath(item);
    lineEdit_PATH->setText(p.path_tesxt);
    applyViewFiltersNow();

    pathHistory.append(p);
    pathHistory_iter = pathHistory.end();

    QModelIndexList deselected_idx = des.indexes();
    QFuture<QModelIndex> dsel_filter = QtConcurrent::filtered(deselected_idx, onlyFirstColumn);
    deselected_idx = dsel_filter.results();
    if (deselected_idx.size() != 1)
        return;

    QModelIndex old_index = treeMapToSource(deselected_idx.at(0));
    const bool switchedToParent = (old_index.parent() == index);

    QModelIndex src;
    if (switchedToParent) {
        FileBrowserItem *old_item = static_cast<FileBrowserItem*>(old_index.internalPointer());
        FileBrowserItem *list_item = list_model->createRootForPath(
                old_item->data(COLUMN_FILEBROWSER_NAME).toString());
        if (list_item)
            src = list_model->index(list_item->row(), 0, QModelIndex());
    } else {
        src = list_model->index(0, 0, QModelIndex());
    }

    QModelIndex i = proxy ? proxy->mapFromSource(src) : src;
    if (i.isValid()) {
        treeView_RPANE->selectionModel()->select(i, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
        treeView_RPANE->selectionModel()->setCurrentIndex(i, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
    }
}
