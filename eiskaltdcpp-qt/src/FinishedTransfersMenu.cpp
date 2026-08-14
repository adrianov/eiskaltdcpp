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

#include "FinishedTransfers.h"
#include "fb2epub/Fb2EpubExport.h"

#include <QAction>
#include <QCursor>
#include <QItemSelectionModel>
#include <QMenu>
#include <QTreeView>

template <bool isUpload>
void FinishedTransfers<isUpload>::slotItemDoubleClicked(const QModelIndex &proxyIndex)
{
    Q_UNUSED(proxyIndex);

    if (comboBox->currentIndex())
        return;

    QItemSelectionModel *s_model = treeView->selectionModel();
    QModelIndexList p_indexes = s_model->selectedRows(0);
    QModelIndexList indexes;

    for (const auto &i : p_indexes)
        indexes.push_back(proxy->mapToSource(i));

    if (indexes.size() < 1)
        return;

    QStringList files;
    FinishedTransfersItem *item = nullptr;
    QString file;
    bool full;

    for (const auto &i : indexes){
        item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
        file = item->data(COLUMN_FINISHED_TARGET).toString();
        full = item->data(COLUMN_FINISHED_FULL).toBool();

        if (!file.isEmpty() && full)
            files.push_back(file);
    }

    for (const auto &f : files) {
        const QString path = finishedLocalPath(f);
        if (Fb2EpubExport::convertIsDefaultOpen() && Fb2EpubExport::isFb2Name(path))
            Fb2EpubExport::convertAndReveal(QStringList{path});
        else
            openOrReveal(f);
    }
}

namespace {

QModelIndexList finishedSourceRows(QTreeView *tree, FinishedTransferProxyModel *proxy)
{
    QModelIndexList indexes;
    for (const auto &i : tree->selectionModel()->selectedRows(0))
        indexes.push_back(proxy->mapToSource(i));
    return indexes;
}

void fillFinishedFiles(const QModelIndexList &indexes, int combo, bool upload,
                       QStringList &files, bool &hasFb2)
{
    for (const auto &i : indexes) {
        auto *item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
        if (!item)
            continue;
        if (combo == 0) {
            const QString file = item->data(COLUMN_FINISHED_TARGET).toString();
            if (file.isEmpty())
                continue;
            files.push_back(file);
            if (!upload && (Fb2EpubExport::isFb2Name(file)
                    || Fb2EpubExport::isFb2Name(item->data(COLUMN_FINISHED_NAME).toString())))
                hasFb2 = true;
            continue;
        }
        const QString file_list = item->data(COLUMN_FINISHED_PATH).toString();
        if (file_list.isEmpty())
            continue;
        const QStringList parts = file_list.split("; ", WULFOR_SKIP_EMPTY);
        files.append(parts);
        if (upload || hasFb2)
            continue;
        for (const auto &part : parts) {
            if (Fb2EpubExport::isFb2Name(part)) {
                hasFb2 = true;
                break;
            }
        }
    }
}

void copyFinishedNames(const QModelIndexList &indexes)
{
    QString names;
    for (const auto &i : indexes) {
        auto *item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
        if (!item)
            continue;
        const QString name = item->data(COLUMN_FINISHED_TARGET).toString().trimmed();
        if (!name.isEmpty())
            names += name + QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(names);
}

} // namespace

template <bool isUpload>
void FinishedTransfers<isUpload>::applyMenuAction(QAction *ret, const QStringList &files,
        const QModelIndexList &indexes, QAction *open_f, QAction *open_dir,
        QAction *copy_name, QAction *delete_f, QAction *convert_epub)
{
    if (ret == open_f) {
        for (const auto &f : files)
            openFile(f);
        return;
    }
    if (ret == open_dir) {
        for (const auto &f : files)
            WulforUtil::revealPath(finishedLocalPath(f));
        return;
    }
    if (convert_epub && ret == convert_epub) {
        QStringList resolved;
        resolved.reserve(files.size());
        for (const auto &f : files)
            resolved.push_back(finishedLocalPath(f));
        Fb2EpubExport::convertAndReveal(resolved);
        return;
    }
    if (copy_name && ret == copy_name)
        copyFinishedNames(indexes);
    else if (delete_f && ret == delete_f)
        deleteDiskFiles(files);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotContextMenu()
{
    static WulforUtil *WU = WulforUtil::getInstance();

    const QModelIndexList indexes = finishedSourceRows(treeView, proxy);
    if (indexes.isEmpty())
        return;

    QStringList files;
    bool hasFb2 = false;
    fillFinishedFiles(indexes, comboBox->currentIndex(), isUpload, files, hasFb2);

    QMenu *m = new QMenu();
    QAction *open_f   = new QAction(WU->getPixmap(AppIcons::eiFOLDER_BLUE), tr("Open file"), m);
    QAction *open_dir = new QAction(WU->getPixmap(AppIcons::eiFOLDER_BLUE), tr("Open directory"), m);
    QAction *convert_epub = nullptr;
    QAction *copy_name = nullptr;
    QAction *delete_f = nullptr;

    m->addAction(open_f);
    m->addAction(open_dir);

    if (!isUpload && hasFb2) {
        convert_epub = new QAction(WU->getPixmap(AppIcons::eiCONVERT_EPUB), tr("Convert to EPUB"), m);
        m->addAction(convert_epub);
        if (Fb2EpubExport::convertIsDefaultOpen())
            m->setDefaultAction(convert_epub);
    }
    if (!m->defaultAction())
        m->setDefaultAction(open_f);

    if (comboBox->currentIndex() == 0){
        copy_name = new QAction(WU->getPixmap(AppIcons::eiEDITCOPY), tr("Copy file name"), m);
        m->addAction(copy_name);

        delete_f = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE), tr("Delete File"), m);
        m->addSeparator();
        m->addAction(delete_f);
    }

    QAction *ret = m->exec(QCursor::pos());
    applyMenuAction(ret, files, indexes, open_f, open_dir, copy_name, delete_f, convert_epub);
    delete m;
}

template void FinishedTransfers<true>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<false>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<true>::applyMenuAction(QAction*, const QStringList&,
        const QModelIndexList&, QAction*, QAction*, QAction*, QAction*, QAction*);
template void FinishedTransfers<false>::applyMenuAction(QAction*, const QStringList&,
        const QModelIndexList&, QAction*, QAction*, QAction*, QAction*, QAction*);
template void FinishedTransfers<true>::slotContextMenu();
template void FinishedTransfers<false>::slotContextMenu();
