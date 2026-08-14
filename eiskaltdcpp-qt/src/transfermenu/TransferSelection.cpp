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

#include "transfermenu/TransferSelection.h"
#include "transfermenu/TransferViewPath.h"
#include "fb2epub/Fb2EpubExport.h"
#include "HubFrame.h"
#include "HubManager.h"
#include "WulforUtil.h"

#include "dcpp/CID.h"

#include <QDesktopServices>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QUrl>

TransferSelection::TransferSelection(TransferView &v, QTreeView *tree)
    : view(v)
{
    if (!tree || !tree->selectionModel())
        return;

    for (const auto &index : tree->selectionModel()->selectedRows(0)) {
        auto *i = reinterpret_cast<TransferViewItem*>(index.internalPointer());
        if (!i)
            continue;
        if (i->childCount() > 0) {
            for (auto *child : i->childItems)
                items.append(child);
        } else if (!items.contains(i)) {
            items.append(i);
        }
    }

    for (auto *i : items) {
        const QString path = TransferViewPath::resolveTransferPath(i);
        if (!path.isEmpty() && !paths.contains(path))
            paths.append(path);
    }
}

bool TransferSelection::canConvert() const
{
    if (paths.isEmpty())
        return false;
    for (const auto &path : paths) {
        if (Fb2EpubExport::isFb2Name(path))
            return true;
    }
    for (auto *i : items) {
        if (Fb2EpubExport::isFb2Name(i->data(COLUMN_TRANSFER_FNAME).toString())
                || Fb2EpubExport::isFb2Name(i->target))
            return true;
    }
    return false;
}

bool TransferSelection::canRemove() const
{
    for (auto *i : items) {
        if (view.canRemoveItem(i))
            return true;
    }
    return false;
}

void TransferSelection::activateFiles() const
{
    if (!paths.isEmpty())
        Fb2EpubExport::activateFiles(paths);
}

void TransferSelection::copyNames() const
{
    QString names;
    for (auto *i : items) {
        QString name = i->target.trimmed();
        if (name.isEmpty())
            name = i->data(COLUMN_TRANSFER_FNAME).toString().trimmed();
        if (!name.isEmpty())
            names += name + QLatin1Char('\n');
    }
    WulforUtil::copyClipboard(names);
}

void TransferSelection::searchAlternates() const
{
    QStringList tths;
    for (auto *item : items) {
        const QString tth = view.getTTHFromItem(item);
        if (tth.isEmpty() || tths.contains(tth))
            continue;
        tths.push_back(tth);
        view.searchAlternates(tth);
    }
}

void TransferSelection::sendPM() const
{
    for (auto *i : items) {
        HubFrame *fr = qobject_cast<HubFrame*>(
                HubManager::getInstance()->getHub(i->data(COLUMN_TRANSFER_HOST).toString()));
        if (fr)
            fr->createPMWindow(dcpp::CID(_tq(i->cid)));
    }
}

void TransferSelection::run(TransferView::Menu::Action act, int copyColumn) const
{
    if (runFile(act, copyColumn) || runPeer(act))
        return;
    runQueue(act);
}

bool TransferSelection::runFile(TransferView::Menu::Action act, int copyColumn) const
{
    switch (act) {
    case TransferView::Menu::OpenFile:
        for (const auto &path : paths)
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        return true;
    case TransferView::Menu::OpenDirectory:
        for (const auto &path : paths)
            WulforUtil::revealPath(path);
        return true;
    case TransferView::Menu::ConvertEpub:
        Fb2EpubExport::convertAndReveal(paths);
        return true;
    case TransferView::Menu::CopyFileName:
        copyNames();
        return true;
    case TransferView::Menu::Copy:
        view.copyMenuSelection(items, copyColumn);
        return true;
    case TransferView::Menu::Remove:
        view.removeMenuSelection(items);
        return true;
    default:
        return false;
    }
}

bool TransferSelection::runPeer(TransferView::Menu::Action act) const
{
    switch (act) {
    case TransferView::Menu::Browse:
        for (auto *i : items)
            view.getFileList(i->cid, i->data(COLUMN_TRANSFER_HOST).toString());
        return true;
    case TransferView::Menu::SearchAlternates:
        searchAlternates();
        return true;
    case TransferView::Menu::MatchQueue:
        for (auto *i : items)
            view.matchQueue(i->cid, i->data(COLUMN_TRANSFER_HOST).toString());
        return true;
    case TransferView::Menu::AddToFav:
        for (auto *i : items)
            view.addFavorite(i->cid);
        return true;
    case TransferView::Menu::GrantExtraSlot:
        for (auto *i : items)
            view.grantSlot(i->cid, i->data(COLUMN_TRANSFER_HOST).toString());
        return true;
    case TransferView::Menu::SendPM:
        sendPM();
        return true;
    default:
        return false;
    }
}

void TransferSelection::runQueue(TransferView::Menu::Action act) const
{
    switch (act) {
    case TransferView::Menu::RemoveFromQueue:
        for (auto *i : items)
            view.removeFromQueue(i->cid);
        break;
    case TransferView::Menu::Force:
        for (auto *i : items)
            view.forceAttempt(i->cid);
        break;
    case TransferView::Menu::Close:
        for (auto *i : items)
            view.closeConection(i->cid, i->download);
        break;
    case TransferView::Menu::showTransferredFieldsOnly:
        view.model->setShowTranferedFilesOnlyState(
                !view.model->getShowTranferedFilesOnlyState());
        break;
    default:
        break;
    }
}
