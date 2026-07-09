/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QItemSelectionModel>
#include <QMenu>
#include <QUrl>

template <bool isUpload>
void FinishedTransfers<isUpload>::openFile(QString file)
{
    if (!file.startsWith(QChar('/')))
        file.prepend("/");

    int sep = file.lastIndexOf(QDir::separator());
    QString name = file.right(sep);
    QString path = file.left(sep);

    QDir test(path);
    if (!test.exists(file)){
        QStringList files = test.entryList(QStringList("*"+name+"*"), QDir::Files, QDir::Name);

        if (files.size() > 0)
            file = path + QDir::separator() + files.first();
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}

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
        openFile(f);
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

    if (comboBox->currentIndex() == 0){
        FinishedTransfersItem *item = nullptr;
        QString file;

        for (const auto &i : indexes){
            item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            file = item->data(COLUMN_FINISHED_TARGET).toString();

            if (!file.isEmpty())
                files.push_back(file);
        }
    }
    else {
        FinishedTransfersItem *item = nullptr;
        QString file_list;

        for (const auto &i : indexes){
            item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            file_list = item->data(COLUMN_FINISHED_PATH).toString();

            if (!file_list.isEmpty()){
                files.append(file_list.split("; ", WULFOR_SKIP_EMPTY));
            }

        }
    }

    QMenu *m = new QMenu();
    QAction *open_f   = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), tr("Open file"), m);
    QAction *open_dir = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), tr("Open directory"), m);
    QAction *copy_name = nullptr;

    m->addAction(open_f);
    m->addAction(open_dir);

    if (comboBox->currentIndex() == 0){
        copy_name = new QAction(WU->getPixmap(WulforUtil::eiEDITCOPY), tr("Copy file name"), m);
        m->addAction(copy_name);
    }

    QAction *ret = m->exec(QCursor::pos());

    delete m;

    if (ret == open_f){
        for (const auto &f : files)
            openFile(f);
    }
    else if (ret == open_dir){
        for (const auto &f : files)
            WulforUtil::revealPath(f);
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
}

template void FinishedTransfers<true>::openFile(QString);
template void FinishedTransfers<false>::openFile(QString);
template void FinishedTransfers<true>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<false>::slotItemDoubleClicked(const QModelIndex&);
template void FinishedTransfers<true>::slotContextMenu();
template void FinishedTransfers<false>::slotContextMenu();
