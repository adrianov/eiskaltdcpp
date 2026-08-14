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

#include "dcpp/ClientManager.h"
#include "dcpp/ShareManager.h"

#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

using namespace dcpp;

namespace {

bool underDir(DirectoryListing::Directory *anc, DirectoryListing::Directory *d)
{
    for (; d; d = d->getParent()) {
        if (d == anc)
            return true;
    }
    return false;
}

bool listingHasName(DirectoryListing::Directory *parent, DirectoryListing::Directory *skip,
                    const string &name)
{
    if (!parent)
        return false;
    for (auto *d : parent->directories) {
        if (d != skip && Util::stricmp(d->getName(), name) == 0)
            return true;
    }
    return false;
}

bool renameListingDir(DirectoryListing::Directory *dir, const string &newName)
{
    DirectoryListing::Directory *parent = dir ? dir->getParent() : nullptr;
    if (!parent)
        return false;
    if (listingHasName(parent, dir, newName))
        return false;
    parent->directories.erase(dir);
    dir->setName(newName);
    parent->directories.insert(dir);
    return true;
}

FileBrowserItem *findDirItem(FileBrowserItem *root, DirectoryListing::Directory *dir)
{
    if (!root || !dir)
        return nullptr;
    if (root->dir == dir)
        return root;
    for (FileBrowserItem *child : root->childItems) {
        if (FileBrowserItem *found = findDirItem(child, dir))
            return found;
    }
    return nullptr;
}

QString stripSlash(QString path)
{
    while (path.endsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('\\')))
        path.chop(1);
    return path;
}

bool renameOnDisk(const QString &oldPath, const QString &newName)
{
    const QFileInfo fi(stripSlash(oldPath));
    QDir parent(fi.absolutePath());
    const QString oldName = fi.fileName();
    if (oldName.isEmpty() || oldName == newName)
        return false;

    const QString dest = parent.filePath(newName);
    const bool caseOnly = (QString::compare(oldName, newName, Qt::CaseInsensitive) == 0);
    if (QFileInfo::exists(dest) && !caseOnly)
        return false;

    if (caseOnly && oldName != newName) {
        const QString tmp = oldName + QLatin1String(".rename");
        if (QFileInfo::exists(parent.filePath(tmp)))
            return false;
        return parent.rename(oldName, tmp) && parent.rename(tmp, newName);
    }
    return parent.rename(oldName, newName);
}

} // namespace

void ShareBrowser::renameOwnFolder(const QModelIndexList &list)
{
    if (user != ClientManager::getInstance()->getMe() || list.size() != 1)
        return;

    FileBrowserItem *item = reinterpret_cast<FileBrowserItem*>(list.at(0).internalPointer());
    if (!item || !item->dir || item->dir == listing.getRoot()
            || dynamic_cast<DirectoryListing::AdlDirectory*>(item->dir))
        return;

    DirectoryListing::Directory *dir = item->dir;
    const StringList paths = listing.getLocalPaths(dir);
    if (paths.size() != 1)
        return;

    const QString oldPath = _q(paths.front());
    const QString oldName = _q(dir->getName());
    bool ok = false;
    const QString next = QInputDialog::getText(this, tr("Rename Folder"), tr("Name"),
                                               QLineEdit::Normal, oldName, &ok).trimmed();
    if (!ok || next.isEmpty() || next == oldName)
        return;
    if (next.contains(QLatin1Char('/')) || next.contains(QLatin1Char('\\'))
            || next == QLatin1String(".") || next == QLatin1String("..")) {
        QMessageBox::warning(this, tr("Rename Folder"), tr("Invalid folder name."));
        return;
    }

    if (listingHasName(dir->getParent(), dir, _tq(next))) {
        QMessageBox::warning(this, tr("Rename Folder"), tr("A folder with that name already exists."));
        return;
    }

    DirectoryListing::Directory *viewed = currentDir();
    if (!renameOnDisk(oldPath, next)) {
        QMessageBox::warning(this, tr("Rename Folder"), tr("Could not rename the folder."));
        return;
    }

    if (!ShareManager::getInstance()->renameDir(paths.front(), _tq(next))) {
        renameOnDisk(QDir(QFileInfo(stripSlash(oldPath)).absolutePath()).filePath(next), oldName);
        QMessageBox::warning(this, tr("Rename Folder"), tr("Could not rename the folder."));
        return;
    }

    if (!renameListingDir(dir, _tq(next)))
        return;

    if (FileBrowserItem *ti = findDirItem(tree_root, dir)) {
        ti->updateColumn(COLUMN_FILEBROWSER_NAME, next);
        tree_model->sort();
    }

    for (SelPair &p : pathHistory) {
        if (!p.dir || (p.dir != dir && !underDir(dir, p.dir)))
            continue;
        QString path = _q(listing.getPath(p.dir));
        p.path_tesxt = stripSlash(path);
    }

    if (viewed) {
        if (FileBrowserItem *ti = findDirItem(tree_root, viewed))
            lineEdit_PATH->setText(tree_model->createRemotePath(ti));
        else
            lineEdit_PATH->setText(stripSlash(_q(listing.getPath(viewed))));
        reloadRightPane(viewed);
    }
    updateUpButton();
}
