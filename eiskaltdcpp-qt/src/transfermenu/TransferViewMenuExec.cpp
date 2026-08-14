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

#include "TransferView.h"
#include "TransferViewPath.h"
#include "fb2epub/Fb2EpubExport.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"

#include <QDesktopServices>
#include <QItemSelectionModel>

namespace {

QList<TransferViewItem*> selectedTransferItems(QTreeView *view) {
    QList<TransferViewItem*> items;
    if (!view || !view->selectionModel())
        return items;

    for (const auto &index : view->selectionModel()->selectedRows(0)) {
        TransferViewItem *i = reinterpret_cast<TransferViewItem*>(index.internalPointer());
        if (!i)
            continue;
        if (i->childCount() > 0) {
            for (const auto &child : i->childItems)
                items.append(child);
        } else if (!items.contains(i)) {
            items.append(i);
        }
    }
    return items;
}

QStringList transferPaths(const QList<TransferViewItem*> &items) {
    QStringList paths;
    for (const auto &i : items) {
        const QString path = TransferViewPath::resolveTransferPath(i);
        if (!path.isEmpty() && !paths.contains(path))
            paths.append(path);
    }
    return paths;
}

bool selectionHasFb2(const QList<TransferViewItem*> &items, const QStringList &paths) {
    for (const auto &path : paths) {
        if (Fb2EpubExport::isFb2Name(path))
            return true;
    }
    for (const auto &i : items) {
        if (Fb2EpubExport::isFb2Name(i->data(COLUMN_TRANSFER_FNAME).toString())
                || Fb2EpubExport::isFb2Name(i->target))
            return true;
    }
    return false;
}

} // namespace

void TransferView::slotDoubleClicked(const QModelIndex &){
    const QList<TransferViewItem*> items = selectedTransferItems(treeView_TRANSFERS);
    const QStringList paths = transferPaths(items);
    if (!paths.isEmpty())
        Fb2EpubExport::activateFiles(paths);
}

void TransferView::slotContextMenu(const QPoint &){
    const QList<TransferViewItem*> items = selectedTransferItems(treeView_TRANSFERS);
    if (items.isEmpty())
        return;

    const QStringList paths = transferPaths(items);
    const bool openEnabled = !paths.isEmpty();
    const bool convertEnabled = openEnabled && selectionHasFb2(items, paths);
    bool removeEnabled = false;
    for (const auto &i : items) {
        if (canRemoveItem(i)) {
            removeEnabled = true;
            break;
        }
    }

    Menu::Action act;
    Menu m(model->getShowTranferedFilesOnlyState(), openEnabled, removeEnabled, convertEnabled);

    act = m.exec();

    switch (act){

    case Menu::None:
    {
        break;
    }
    case Menu::Browse:
    {
        for (const auto &i : items)
            getFileList(i->cid, vstr(i->data(COLUMN_TRANSFER_HOST)));

        break;
    }
    case Menu::OpenFile:
    {
        for (const auto &path : paths)
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));

        break;
    }
    case Menu::OpenDirectory:
    {
        for (const auto &path : paths)
            WulforUtil::revealPath(path);

        break;
    }
    case Menu::ConvertEpub:
    {
        Fb2EpubExport::convertAndReveal(paths);
        break;
    }
    case Menu::SearchAlternates:
    {
        QStringList tths;
        QString tth_str = "";
        for (const auto &item : items) {
            tth_str = getTTHFromItem(item);
            if (!tth_str.isEmpty() && !tths.contains(tth_str)){
                tths.push_back(tth_str);
                searchAlternates(tth_str);
            }
        }

        break;
    }
    case Menu::MatchQueue:
    {
        for (const auto &i : items)
            matchQueue(i->cid, vstr(i->data(COLUMN_TRANSFER_HOST)));

        break;
    }
    case Menu::AddToFav:
    {
        for (const auto &i : items)
            addFavorite(i->cid);

        break;
    }
    case Menu::GrantExtraSlot:
    {
        for (const auto &i : items)
            grantSlot(i->cid, vstr(i->data(COLUMN_TRANSFER_HOST)));

        break;
    }
    case Menu::CopyFileName:
    {
        copyMenuSelection(items, COLUMN_TRANSFER_FNAME);
        break;
    }
    case Menu::Copy:
    {
        copyMenuSelection(items, m.copyColumn());
        break;
    }
    case Menu::RemoveFromQueue:
    {
        for (const auto &i : items)
            removeFromQueue(i->cid);

        break;
    }
    case Menu::Remove:
    {
        removeMenuSelection(items);
        break;
    }
    case Menu::Force:
    {
        for (const auto &i : items)
            forceAttempt(i->cid);

        break;
    }
    case Menu::showTransferredFieldsOnly:
    {
        model->setShowTranferedFilesOnlyState(!model->getShowTranferedFilesOnlyState());
        break;
    }
    case Menu::Close:
    {
        for (const auto &i : items)
            closeConection(i->cid, i->download);

        break;
    }
    case Menu::SendPM:
    {
        HubFrame *fr = nullptr;

        for (const auto &i : items){
            dcpp::CID cid(_tq(i->cid));
            QString hubUrl = i->data(COLUMN_TRANSFER_HOST).toString();

            fr = qobject_cast<HubFrame*>(HubManager::getInstance()->getHub(hubUrl));

            if (fr)
                fr->createPMWindow(cid);
        }

        break;
    }
    default:
        break;
    }
}
