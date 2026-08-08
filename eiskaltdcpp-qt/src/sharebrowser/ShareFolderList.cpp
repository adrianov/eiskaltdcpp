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
#include "WulforSettings.h"
#include "FileBrowserModel.h"

#include <QDateTime>
#include <QHeaderView>

using namespace dcpp;

namespace {

QList<QVariant> fileRowData(DirectoryListing::File *file)
{
    return QList<QVariant>()
            << _q(file->getName())
            << WulforUtil::formatBytes(file->getSize())
            << static_cast<quint64>(file->getSize())
            << _q(file->getTTH().toBase32())
            << file->mediaInfo.bitrate
            << _q(file->mediaInfo.resolution)
            << _q(file->mediaInfo.video_info)
            << _q(file->mediaInfo.audio_info)
            << static_cast<quint64>(file->getHit())
            << QDateTime::fromTime_t(file->getTS()).toString("yyyy-MM-dd hh:mm");
}

void appendFlatFiles(FileBrowserItem *listRoot, DirectoryListing &listing,
                     DirectoryListing::Directory *dir, quint64 &totalSize)
{
    if (!dir)
        return;

    const QString path = _q(listing.getPath(dir));
    for (const auto &file : dir->files) {
        totalSize += file->getSize();
        QList<QVariant> data = fileRowData(file);
        data << path;
        FileBrowserItem *child = new FileBrowserItem(data, listRoot);
        child->file = file;
        listRoot->appendChild(child);
    }
    for (const auto &sub : dir->directories)
        appendFlatFiles(listRoot, listing, sub, totalSize);
}

} // namespace

void ShareBrowser::changeRoot(DirectoryListing::Directory *root){
    if (!root)
        return;

    list_model->beginRebuild();
    current_size = 0;

    for (const auto &dir : root->directories) {
        const quint64 size = dir->getTotalSize(true);
        current_size += size;
        QList<QVariant> data;
        data << _q(dir->getName()) << WulforUtil::formatBytes(size) << size << "";
        FileBrowserItem *child = new FileBrowserItem(data, list_root);
        child->dir = dir;
        list_root->appendChild(child);
    }

    for (const auto &file : root->files) {
        current_size += file->getSize();
        FileBrowserItem *child = new FileBrowserItem(fileRowData(file), list_root);
        child->file = file;
        list_root->appendChild(child);
    }

    list_model->highlightDuplicates();
    list_model->endRebuild();
    label_RIGHT->setText(QString(tr("Total size: %1")).arg(WulforUtil::formatBytes(current_size)));
}

void ShareBrowser::changeRootFlat(DirectoryListing::Directory *root){
    if (!root)
        return;

    list_model->beginRebuild();
    current_size = 0;
    appendFlatFiles(list_root, listing, root, current_size);
    list_model->highlightDuplicates();
    list_model->endRebuild();
    label_RIGHT->setText(QString(tr("Total size: %1")).arg(WulforUtil::formatBytes(current_size)));
}

void ShareBrowser::reloadRightPane(DirectoryListing::Directory *dir){
    if (!dir)
        return;
    if (flatMode)
        changeRootFlat(dir);
    else
        changeRoot(dir);
}

DirectoryListing::Directory *ShareBrowser::currentDir() {
    if (treeView_LPANE && treeView_LPANE->selectionModel()) {
        const QModelIndexList selected = treeView_LPANE->selectionModel()->selectedRows(0);
        if (selected.size() == 1) {
            const QModelIndex index = treeMapToSource(selected.at(0));
            if (index.isValid()) {
                FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());
                if (item && item->dir)
                    return item->dir;
            }
        }
    }

    // Path bar matches the folder in view when the left selection is empty/multi.
    if (!lineEdit_PATH->text().isEmpty()) {
        FileBrowserItem *item = tree_model->createRootForPath(lineEdit_PATH->text());
        if (item && item->dir)
            return item->dir;
    }

    return listing.getRoot();
}

void ShareBrowser::applyFlatMode(bool on){
    flatMode = on;
    frame_2->setVisible(!on);
    toolButton_BACK->setEnabled(!on);
    toolButton_FORWARD->setEnabled(!on);
    toolButton_UP->setEnabled(!on);
    treeView_RPANE->header()->setSectionHidden(COLUMN_FILEBROWSER_PATH, !on);
    reloadRightPane(currentDir());
    applyViewFiltersNow();
}

void ShareBrowser::slotFlatToggled(bool on){
    WBSET(WB_SHARE_FLAT, on);
    applyFlatMode(on);
}
