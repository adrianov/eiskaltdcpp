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
#include "MainWindow.h"

#include "dcpp/SettingsManager.h"

#include <QDateTime>
#include <QAbstractItemView>
#include <QFuture>

#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#else
#include <QtConcurrentFilter>
#endif

using namespace dcpp;

void ShareBrowser::changeRoot(dcpp::DirectoryListing::Directory *root){
    if (!root)
        return;

    list_model->clear();

    current_size = 0;

    for (const auto &dir : root->directories){
        FileBrowserItem *child;
        quint64 size = 0;
        QList<QVariant> data;

        size = dir->getTotalSize(true);
        current_size += size;

        data << _q(dir->getName())
             << WulforUtil::formatBytes(size)
             << size
             << "";

        child = new FileBrowserItem(data, list_root);
        child->dir = dir;

        list_root->appendChild(child);
    }

    for (const auto& file : root->files){
        FileBrowserItem *child;
        quint64 size = 0;
        QList<QVariant> data;

        size = file->getSize();
        current_size += size;

        data << _q(file->getName())
             << WulforUtil::formatBytes(size)
             << size
             << _q(file->getTTH().toBase32())
             << file->mediaInfo.bitrate
             << _q(file->mediaInfo.resolution)
             << _q(file->mediaInfo.video_info)
             << _q(file->mediaInfo.audio_info)
             << (quint64)file->getHit()
             << QDateTime::fromTime_t(file->getTS()).toString("yyyy-MM-dd hh:mm");

        child = new FileBrowserItem(data, list_root);
        child->file = file;

        list_root->appendChild(child);
    }

    label_RIGHT->setText(QString(tr("Total size: %1")).arg(WulforUtil::formatBytes(current_size)));

    list_model->highlightDuplicates();

    list_model->sort();

    // changeRoot appends without insert signals; remap proxy under current filters.
    if (proxy)
        proxy->invalidate();
    if (tree_proxy)
        tree_proxy->invalidate();
}

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

    status += tr("Total size: %1").arg(WulforUtil::formatBytes(current_size));

    if (selected_size > 0)
        status += tr("; Selected: %1").arg(WulforUtil::formatBytes(selected_size));

    label_RIGHT->setText(status);
}

static bool onlyFirstColumn(const QModelIndex &index){
    return (index.column() == 0);
}

void ShareBrowser::slotLeftPaneSelChanged(const QItemSelection &sel, const QItemSelection &des){
    Q_UNUSED(sel)

    QItemSelectionModel *selection_model = treeView_LPANE->selectionModel();
    QModelIndexList selected  = selection_model->selectedRows(0);

    // Multi-select (Shift/Ctrl) keeps the current listing; browse only on a single selection.
    if (selected.size() != 1)
        return;

    QModelIndex index = selected.at(0);
    index = treeMapToSource(index);

    if (index.isValid()){

        SelPair p;

        FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());

        changeRoot(item->dir);
        p.dir = item->dir;
        p.index = index;

        lineEdit_PATH->setText(tree_model->createRemotePath(item));
        p.path_tesxt = tree_model->createRemotePath(item);
        applyViewFiltersNow();

        pathHistory.append(p);
        pathHistory_iter = pathHistory.end();

        QModelIndexList deselected_idx = des.indexes();
        QFuture<QModelIndex> dsel_filter    = QtConcurrent::filtered(deselected_idx, onlyFirstColumn);

        deselected_idx  = dsel_filter.results();

        if (deselected_idx.size() != 1)
            return;

        QModelIndex old_index = treeMapToSource(deselected_idx.at(0));
        bool switchedToParent = (old_index.parent() == index);

        QModelIndex src;
        if (switchedToParent){
            FileBrowserItem *old_item = static_cast<FileBrowserItem*>(old_index.internalPointer());
            FileBrowserItem *list_item = list_model->createRootForPath(
                    old_item->data(COLUMN_FILEBROWSER_NAME).toString());
            if (list_item)
                src = list_model->index(list_item->row(), 0, QModelIndex());
        } else {
            src = list_model->index(0, 0, QModelIndex());
        }

        QModelIndex i = proxy ? proxy->mapFromSource(src) : src;
        if (i.isValid()){
            treeView_RPANE->selectionModel()->select(i, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
            treeView_RPANE->selectionModel()->setCurrentIndex(i, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
        }
    }
}
