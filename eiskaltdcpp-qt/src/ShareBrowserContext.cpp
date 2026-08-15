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

#include "ShareBrowser.h"
#include "FileBrowserModel.h"
#include "downloadto/DownloadToHistory.h"
#include "fb2epub/Fb2EpubExport.h"

#include "dcpp/SettingsManager.h"

#include <QFileDialog>
#include <QDir>
#include <QSet>

using namespace dcpp;

namespace {

DirectoryListing::Directory *wholeDir(FileBrowserItem *item)
{
    if (!item)
        return nullptr;
    if (item->dir)
        return item->dir;
    if (item->file)
        return item->file->getParent();
    return nullptr;
}

QString pickDownloadTarget(QWidget *parent, const QString &title, const QString &chosen, QString &oldTarget)
{
    QString target = chosen;

    if (!QDir(target).exists() || target.isEmpty())
        target = QFileDialog::getExistingDirectory(parent, title, oldTarget);

    if (target.isEmpty())
        return QString();

    target = QDir::toNativeSeparators(target);

    if (!target.endsWith(QDir::separator()))
        target += QDir::separator();

    oldTarget = target;

    QStringList temp_pathes = DownloadToDirHistory::get();
    temp_pathes.push_front(target);
    DownloadToDirHistory::put(temp_pathes);

    return target;
}

FileBrowserItem *itemAt(const QModelIndex &index)
{
    return reinterpret_cast<FileBrowserItem*>(index.internalPointer());
}

} // namespace

QModelIndexList ShareBrowser::mappedSelection(QTreeView *view) const
{
    const QModelIndexList selected = view->selectionModel()->selectedRows(0);
    QModelIndexList list;
    if (view == treeView_RPANE && proxy) {
        for (const QModelIndex &i : selected)
            list.push_back(proxy->mapToSource(i));
        return list;
    }
    if (view == treeView_LPANE && tree_proxy) {
        for (const QModelIndex &i : selected)
            list.push_back(tree_proxy->mapToSource(i));
        return list;
    }
    return selected;
}

ShareBrowserMenu::Flags ShareBrowser::menuFlags(QTreeView *view, const QModelIndexList &list)
{
    ShareBrowserMenu::Flags flags;
    flags.treePane = (view == treeView_LPANE);
    for (const auto &index : list) {
        FileBrowserItem *item = itemAt(index);
        if (!item)
            continue;
        if (item->file) {
            flags.deletable = true;
            if (Fb2EpubExport::isFb2Name(_q(item->file->getName())))
                flags.fb2 = true;
        }
        if (nestedDeleteDir(item))
            flags.deleteWholeDir = true;
    }
    if (list.size() != 1)
        return flags;
    FileBrowserItem *item = itemAt(list.at(0));
    if (item && item->dir && item->dir != listing.getRoot()
            && !dynamic_cast<DirectoryListing::AdlDirectory*>(item->dir)
            && listing.getLocalPaths(item->dir).size() == 1)
        flags.renameFolder = true;
    return flags;
}

void ShareBrowser::contextDownload(ShareBrowserMenu::Action act, const QModelIndexList &list)
{
    const bool whole = (act == ShareBrowserMenu::DownloadWholeDir
                        || act == ShareBrowserMenu::DownloadWholeDirTo);
    const bool choose = (act == ShareBrowserMenu::DownloadTo
                         || act == ShareBrowserMenu::DownloadWholeDirTo);
    QString target = _q(SETTING(DOWNLOAD_DIRECTORY));
    if (choose) {
        static QString oldDownload = QDir::homePath();
        static QString oldWhole = QDir::homePath();
        QString &old = whole ? oldWhole : oldDownload;
        target = pickDownloadTarget(this, tr("Select directory"),
                                    ShareBrowserMenu::getInstance()->getTarget(), old);
        if (target.isEmpty())
            return;
    }
    if (whole) {
        QSet<DirectoryListing::Directory*> dirs;
        for (const auto &index : list) {
            if (DirectoryListing::Directory *dir = wholeDir(itemAt(index)))
                dirs.insert(dir);
        }
        for (DirectoryListing::Directory *dir : dirs)
            download(dir, target);
        return;
    }
    for (const auto &index : list) {
        FileBrowserItem *item = itemAt(index);
        if (!item)
            continue;
        if (item->file)
            download(item->file, target);
        else if (item->dir)
            download(item->dir, target);
    }
}

void ShareBrowser::slotCustomContextMenu(const QPoint &)
{
    QTreeView *view = dynamic_cast<QTreeView*>(sender());
    if (!view)
        return;

    const QModelIndexList list = mappedSelection(view);
    if (!ShareBrowserMenu::getInstance())
        ShareBrowserMenu::newInstance();

    const ShareBrowserMenu::Action act =
            ShareBrowserMenu::getInstance()->exec(user, menuFlags(view, list));
    switch (act) {
        case ShareBrowserMenu::None:
            break;
        case ShareBrowserMenu::Download:
        case ShareBrowserMenu::DownloadTo:
        case ShareBrowserMenu::DownloadWholeDir:
        case ShareBrowserMenu::DownloadWholeDirTo:
            contextDownload(act, list);
            break;
        default:
            contextMoreActions(act, list);
            break;
    }
}
