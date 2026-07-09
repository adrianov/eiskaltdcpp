/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "TransferView.h"
#include "TransferViewPath.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"

#include <QDesktopServices>
#include <QItemSelectionModel>

void TransferView::slotContextMenu(const QPoint &){
    QItemSelectionModel *selection_model = treeView_TRANSFERS->selectionModel();
    QModelIndexList list = selection_model->selectedRows(0);

    if (list.size() < 1)
        return;

    QList<TransferViewItem*> items;

    for (const auto &index : list){
        TransferViewItem *i = reinterpret_cast<TransferViewItem*>(index.internalPointer());

        if (i->childCount() > 0){
            for (const auto &child : i->childItems)
                items.append(child);
        }
        else if (!items.contains(i))
            items.append(i);
    }

    if (items.size() < 1)
        return;

    bool openEnabled = false;
    bool removeEnabled = false;
    for (const auto &i : items) {
        if (!openEnabled && !TransferViewPath::resolveTransferPath(i).isEmpty())
            openEnabled = true;
        if (!removeEnabled && canRemoveItem(i))
            removeEnabled = true;
        if (openEnabled && removeEnabled)
            break;
    }

    Menu::Action act;
    Menu m(model->getShowTranferedFilesOnlyState(), openEnabled, removeEnabled);

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
        QStringList paths;
        for (const auto &i : items) {
            const QString path = TransferViewPath::resolveTransferPath(i);
            if (!path.isEmpty() && !paths.contains(path))
                paths.append(path);
        }
        for (const auto &path : paths)
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));

        break;
    }
    case Menu::OpenDirectory:
    {
        QStringList paths;
        for (const auto &i : items) {
            const QString path = TransferViewPath::resolveTransferPath(i);
            if (!path.isEmpty() && !paths.contains(path))
                paths.append(path);
        }
        for (const auto &path : paths)
            WulforUtil::revealPath(path);

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
