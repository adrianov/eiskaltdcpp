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

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QItemSelectionModel>
#include <QMenu>

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

    for (const auto &f : files)
        openOrReveal(f);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotContextMenu()
{
    static WulforUtil *WU = WulforUtil::getInstance();

    QItemSelectionModel *s_model = treeView->selectionModel();
    QModelIndexList p_indexes = s_model->selectedRows(0);
    QModelIndexList indexes;

    for (const auto &i : p_indexes)
        indexes.push_back(proxy->mapToSource(i));

    if (indexes.size() < 1)
        return;

    QStringList files;
    bool hasFb2 = false;

    if (comboBox->currentIndex() == 0){
        for (const auto &i : indexes){
            auto *item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            if (!item)
                continue;
            const QString file = item->data(COLUMN_FINISHED_TARGET).toString();
            if (file.isEmpty())
                continue;
            files.push_back(file);
            // Suffix on TARGET/FNAME only — exists() can fail until finishedLocalPath().
            if (!isUpload && (Fb2EpubExport::isFb2Name(file)
                    || Fb2EpubExport::isFb2Name(item->data(COLUMN_FINISHED_NAME).toString())))
                hasFb2 = true;
        }
    }
    else {
        for (const auto &i : indexes){
            auto *item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            if (!item)
                continue;
            const QString file_list = item->data(COLUMN_FINISHED_PATH).toString();
            if (file_list.isEmpty())
                continue;
            const QStringList parts = file_list.split("; ", WULFOR_SKIP_EMPTY);
            files.append(parts);
            if (!isUpload) {
                for (const auto &part : parts) {
                    if (Fb2EpubExport::isFb2Name(part)) {
                        hasFb2 = true;
                        break;
                    }
                }
            }
        }
    }

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
    }

    if (comboBox->currentIndex() == 0){
        copy_name = new QAction(WU->getPixmap(AppIcons::eiEDITCOPY), tr("Copy file name"), m);
        m->addAction(copy_name);

        delete_f = new QAction(WU->getPixmap(AppIcons::eiEDITDELETE), tr("Delete File"), m);
        m->addSeparator();
        m->addAction(delete_f);
    }

    QAction *ret = m->exec(QCursor::pos());

    delete m;

    if (ret == open_f){
        for (const auto &f : files)
            openFile(f);
    }
    else if (ret == open_dir){
        for (const auto &f : files)
            WulforUtil::revealPath(finishedLocalPath(f));
    }
    else if (convert_epub && ret == convert_epub){
        QStringList resolved;
        resolved.reserve(files.size());
        for (const auto &f : files)
            resolved.push_back(finishedLocalPath(f));
        Fb2EpubExport::convertAndReveal(resolved);
    }
    else if (copy_name && ret == copy_name){
        QString names;

        for (const auto &i : indexes){
            FinishedTransfersItem *item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            const QString name = item->data(COLUMN_FINISHED_NAME).toString().trimmed();

            if (!name.isEmpty())
                names += name + "\n";
        }

        names = names.trimmed();

        if (!names.isEmpty())
            qApp->clipboard()->setText(names, QClipboard::Clipboard);
    }
    else if (delete_f && ret == delete_f){
        deleteDiskFiles(files);
    }
}

template void FinishedTransfers<true>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<false>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<true>::slotContextMenu();
template void FinishedTransfers<false>::slotContextMenu();
