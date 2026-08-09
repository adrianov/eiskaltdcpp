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

#include "settings/SimpleShareTree.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/ShareManager.h"

#include <QAction>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

using namespace dcpp;

SimpleShareTree::SimpleShareTree(QTreeWidget *tree, QWidget *host)
    : QObject(host)
    , tree_(tree)
    , host_(host)
{
}

void SimpleShareTree::refresh()
{
    tree_->clear();
    ShareManager *SM = ShareManager::getInstance();
    for (const auto &pair : SM->getDirectories()) {
        auto *item = new QTreeWidgetItem(tree_);
        const qlonglong size = SM->getShareSize(pair.second);
        item->setText(0, pair.second.c_str());
        item->setText(1, pair.first.c_str());
        item->setText(2, WulforUtil::formatBytes(size));
        item->setText(3, QString::number(size));
    }
}

void SimpleShareTree::showMenu(const QPoint &)
{
    const QList<QTreeWidgetItem *> selected = tree_->selectedItems();
    QMenu menu(host_);
    WulforUtil *WU = WulforUtil::getInstance();

    QAction *add = menu.addAction(WU->getPixmap(AppIcons::eiEDITADD), tr("Add"));
    QAction *rename = nullptr;
    QAction *remove = nullptr;
    if (selected.size() == 1)
        rename = menu.addAction(WU->getPixmap(AppIcons::eiEDIT), tr("Rename"));
    if (!selected.isEmpty())
        remove = menu.addAction(WU->getPixmap(AppIcons::eiEDITDELETE), tr("Remove"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (!chosen)
        return;
    if (chosen == add)
        addDirectory();
    else if (chosen == remove)
        removeSelected();
    else if (chosen == rename)
        renameSelected();
    refresh();
}

void SimpleShareTree::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(host_, tr("Select directory"), QDir::homePath());
    if (dir.isEmpty())
        return;

    dir = QDir::toNativeSeparators(dir);
    if (!dir.endsWith(PATH_SEPARATOR))
        dir += PATH_SEPARATOR_STR;

    bool ok = false;
    QString alias = QInputDialog::getText(host_, tr("Select directory"), tr("Name"),
                                          QLineEdit::Normal, QDir(dir).dirName(), &ok).trimmed();
    if (!ok || alias.isEmpty())
        return;

    try {
        ShareManager::getInstance()->addDirectory(dir.toStdString(), alias.toStdString());
    } catch (const ShareException &e) {
        QMessageBox::critical(host_, tr("Error"), QString::fromStdString(e.getError()));
    }
}

void SimpleShareTree::removeSelected()
{
    for (QTreeWidgetItem *item : tree_->selectedItems())
        ShareManager::getInstance()->removeDirectory(item->text(0).toStdString());
}

void SimpleShareTree::renameSelected()
{
    QTreeWidgetItem *item = tree_->selectedItems().value(0);
    if (!item)
        return;

    const QString realName = item->text(0);
    const QString virtName = item->text(1);
    bool ok = false;
    const QString next = QInputDialog::getText(host_, tr("Enter new name"), tr("Name"),
                                               QLineEdit::Normal, virtName, &ok).trimmed();
    if (!ok || next.isEmpty() || next == virtName)
        return;

    try {
        ShareManager::getInstance()->renameDirectory(realName.toStdString(), next.toStdString());
    } catch (const ShareException &e) {
        QMessageBox::critical(host_, tr("Error"), QString::fromStdString(e.getError()));
    }
}
