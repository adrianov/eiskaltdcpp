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

#include "settings/ShareDirsPane.h"
#include "settings/ShareDirModel.h"
#include "settings/SimpleShareTree.h"
#include "HashProgress.h"
#include "WulforUtil.h"
#include "WulforSettings.h"

#include "dcpp/stdinc.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"

#include <QDir>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTreeView>
#include <QTreeWidget>

using namespace dcpp;

ShareDirsPane::ShareDirsPane(QTreeView *treeView, QTreeWidget *simpleTree, QLabel *totalLabel,
                             QWidget *host)
    : QObject(host)
    , treeView_(treeView)
    , simpleWidget_(simpleTree)
    , totalLabel_(totalLabel)
    , host_(host)
{
    simpleTree_ = new SimpleShareTree(simpleWidget_, host_);
}

void ShareDirsPane::showSimpleMenu(const QPoint &pos)
{
    simpleTree_->showMenu(pos);
    totalLabel_->setText(tr("Total shared: %1")
            .arg(WulforUtil::formatBytes(ShareManager::getInstance()->getShareSize())));
}

void ShareDirsPane::ensureModel()
{
    if (model_)
        return;

    model_ = new ShareDirModel(this);
    treeView_->setModel(model_);
    treeView_->setSortingEnabled(true);
    treeView_->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_->header()->hideSection(1);
    treeView_->header()->hideSection(2);
    if (!WSGET(WS_SHAREHEADER_STATE).isEmpty()) {
        WulforUtil::restoreTreeHeader(treeView_->header(),
                QByteArray::fromBase64(WSGET(WS_SHAREHEADER_STATE).toUtf8()));
    }
    connect(model_, &ShareDirModel::getName, this, &ShareDirsPane::askAlias);
    connect(model_, &ShareDirModel::expandMe, treeView_, &QTreeView::expand);
}

void ShareDirsPane::setSimpleMode(bool simple)
{
    if (!simple) {
        ensureModel();
        model_->beginExpanding();
        return;
    }
    WulforUtil::restoreTreeHeader(simpleWidget_->header(),
            QByteArray::fromBase64(WSGET("settings-simple-share-headerstate").toUtf8()));
    refreshTotals();
}

void ShareDirsPane::refreshTotals()
{
    if (simpleWidget_->isVisible())
        simpleTree_->refresh();
    totalLabel_->setText(tr("Total shared: %1")
            .arg(WulforUtil::formatBytes(ShareManager::getInstance()->getShareSize())));
}

void ShareDirsPane::recreateShare()
{
    ShareManager *SM = ShareManager::getInstance();
    SM->setDirty();
    SM->refresh(true);
    HashProgress progress(host_);
    if (progress.exec() == QDialog::Accepted)
        refreshTotals();
}

void ShareDirsPane::setShareHidden(bool share)
{
    SettingsManager::getInstance()->set(SettingsManager::SHARE_HIDDEN, share);
    ShareManager::getInstance()->setDirty();
    ShareManager::getInstance()->refresh(true);
    refreshTotals();
}

void ShareDirsPane::askAlias(const QModelIndex &index)
{
    if (!model_)
        return;
    bool ok = false;
    const QString alias = QInputDialog::getText(host_, tr("Select directory"), tr("Name"),
            QLineEdit::Normal, QDir(model_->filePath(index)).dirName(), &ok).trimmed();
    if (ok && !alias.isEmpty())
        model_->setAlias(index, alias);
}
