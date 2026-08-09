/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "settings/SettingsSharing.h"
#include "settings/ShareDirsPane.h"
#include "WulforUtil.h"
#include "WulforSettings.h"

#include "dcpp/stdinc.h"
#include "dcpp/SettingsManager.h"

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QListWidgetItem>

using namespace dcpp;

SettingsSharing::SettingsSharing(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    init();
}

void SettingsSharing::showEvent(QShowEvent *e)
{
    e->accept();
    if (!WSGET(WS_SHAREHEADER_STATE).isEmpty()) {
        WulforUtil::restoreTreeHeader(treeView->header(),
                QByteArray::fromBase64(WSGET(WS_SHAREHEADER_STATE).toUtf8()));
    }
}

void SettingsSharing::ok()
{
    SettingsManager *SM = SettingsManager::getInstance();

    SM->set(SettingsManager::FOLLOW_LINKS, checkBox_FOLLOW->isChecked());
    SM->set(SettingsManager::USE_ADL_ONLY_OWN_LIST, checkBox_USE_ADL_ONLY_OWN_LIST->isChecked());
    SM->set(SettingsManager::SHARE_TEMP_FILES, checkBox_SHARE_TEMP_FILES->isChecked());
    SM->set(SettingsManager::MIN_UPLOAD_SPEED, spinBox_EXTRA->value());
    SM->set(SettingsManager::SLOTS_PRIMARY, spinBox_UPLOAD->value());
    SM->set(SettingsManager::MAX_HASH_SPEED, spinBox_MAXHASHSPEED->value());
    SM->set(SettingsManager::FAST_HASH, checkBox_FASTHASH->isChecked());
    SM->set(SettingsManager::AUTO_REFRESH_TIME, spinBox_REFRESH_TIME->value());
    SM->set(SettingsManager::HASHING_START_DELAY, spinBox_HASHING_START_DELAY->value());
    SM->set(SettingsManager::HASH_BUFFER_NORESERVE, checkBox_MAPNORESERVE->isChecked());
    SM->set(SettingsManager::HASH_BUFFER_POPULATE, checkBox_MAPPOPULATE->isChecked());
    SM->set(SettingsManager::HASH_BUFFER_PRIVATE, checkBox_MAPPRIVATE->isChecked());
    SM->set(SettingsManager::HASH_BUFFER_SIZE_MB, comboBox_BUFSIZE->currentText().toInt());
    SM->set(SettingsManager::SHARE_SKIP_ZERO_BYTE, checkBox_SHARE_SKIP_ZERO_BYTE->isChecked());

    QStringList list;
    for (int k = 0; k < listWidget_SKIPLIST->count(); ++k)
        list << listWidget_SKIPLIST->item(k)->text();
    SM->set(SettingsManager::SKIPLIST_SHARE, list.isEmpty() ? "|" : _tq(list.join("|")));

    WBSET(WB_SIMPLE_SHARE_MODE, checkBox_SIMPLE_SHARE_MODE->isChecked());
    if (checkBox_SIMPLE_SHARE_MODE->isChecked())
        SM->save();

    WSSET(WS_SHAREHEADER_STATE, treeView->header()->saveState().toBase64());
    WSSET("settings-simple-share-headerstate", treeWidget_SIMPLE_MODE->header()->saveState().toBase64());
    WBSET(WB_APP_REMOVE_NOT_EX_DIRS, checkBox_AUTOREMOVE->isChecked());
}

void SettingsSharing::init()
{
    WulforUtil *WU = WulforUtil::getInstance();
    toolButton_ADD->setIcon(WU->getPixmap(AppIcons::eiBOOKMARK_ADD));
    toolButton_EDIT->setIcon(WU->getPixmap(AppIcons::eiEDIT));
    toolButton_DELETE->setIcon(WU->getPixmap(AppIcons::eiEDITDELETE));
    toolButton_BROWSE->setIcon(WU->getPixmap(AppIcons::eiFOLDER_BLUE));
    toolButton_RECREATE->setIcon(WU->getPixmap(AppIcons::eiRELOAD));

    checkBox_SHAREHIDDEN->setChecked(BOOLSETTING(SHARE_HIDDEN));
    checkBox_SHARE_TEMP_FILES->setChecked(BOOLSETTING(SHARE_TEMP_FILES));
    checkBox_FOLLOW->setChecked(BOOLSETTING(FOLLOW_LINKS));
    checkBox_USE_ADL_ONLY_OWN_LIST->setChecked(BOOLSETTING(USE_ADL_ONLY_OWN_LIST));
    spinBox_UPLOAD->setValue(SETTING(SLOTS_PRIMARY));
    spinBox_MAXHASHSPEED->setValue(SETTING(MAX_HASH_SPEED));
    spinBox_EXTRA->setValue(SETTING(MIN_UPLOAD_SPEED));
    spinBox_REFRESH_TIME->setValue(SETTING(AUTO_REFRESH_TIME));
    spinBox_HASHING_START_DELAY->setValue(SETTING(HASHING_START_DELAY));
    checkBox_AUTOREMOVE->setChecked(WBGET(WB_APP_REMOVE_NOT_EX_DIRS));
    checkBox_SHARE_SKIP_ZERO_BYTE->setChecked(BOOLSETTING(SHARE_SKIP_ZERO_BYTE));
    checkBox_FASTHASH->setChecked(BOOLSETTING(FAST_HASH));
    groupBox_FASTHASH->setEnabled(BOOLSETTING(FAST_HASH));
    listWidget_SKIPLIST->addItems(_q(SETTING(SKIPLIST_SHARE)).split('|', WULFOR_SKIP_EMPTY));

    const bool simple = WBGET(WB_SIMPLE_SHARE_MODE);
    checkBox_SIMPLE_SHARE_MODE->setChecked(simple);
    treeWidget_SIMPLE_MODE->setVisible(simple);
    treeWidget_SIMPLE_MODE->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->setHidden(simple);

    checkBox_MAPNORESERVE->setChecked(SETTING(HASH_BUFFER_NORESERVE));
    checkBox_MAPPOPULATE->setChecked(SETTING(HASH_BUFFER_POPULATE));
    checkBox_MAPPRIVATE->setChecked(SETTING(HASH_BUFFER_PRIVATE));
    const int ind = comboBox_BUFSIZE->findText(QString::number(SETTING(HASH_BUFFER_SIZE_MB)));
    if (ind >= 0)
        comboBox_BUFSIZE->setCurrentIndex(ind);

    dirs = new ShareDirsPane(treeView, treeWidget_SIMPLE_MODE, label_TOTALSHARED, this);

    connect(toolButton_ADD, SIGNAL(clicked()), this, SLOT(slotAddException()));
    connect(toolButton_EDIT, SIGNAL(clicked()), this, SLOT(slotEditException()));
    connect(toolButton_DELETE, SIGNAL(clicked()), this, SLOT(slotDeleteException()));
    connect(toolButton_BROWSE, SIGNAL(clicked()), this, SLOT(slotAddDirException()));
    connect(listWidget_SKIPLIST, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(slotEditException()));
    connect(toolButton_RECREATE, SIGNAL(clicked()), this, SLOT(slotRecreateShare()));
    connect(checkBox_SHAREHIDDEN, SIGNAL(clicked(bool)), this, SLOT(slotShareHidden(bool)));
    connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotHeaderMenu()));
    connect(treeWidget_SIMPLE_MODE, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotContextMenu(QPoint)));
    connect(checkBox_SIMPLE_SHARE_MODE, SIGNAL(clicked()), this, SLOT(slotSimpleShareModeChanged()));

    slotSimpleShareModeChanged();
    dirs->refreshTotals();
}

void SettingsSharing::slotRecreateShare()
{
    dirs->recreateShare();
}

void SettingsSharing::slotShareHidden(bool share)
{
    dirs->setShareHidden(share);
}

void SettingsSharing::slotHeaderMenu()
{
    WulforUtil::headerMenu(treeView);
}

void SettingsSharing::slotAddException()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Add item"), tr("Enter text:"),
                                               QLineEdit::Normal, QString(), &ok);
    if (ok && !text.isEmpty())
        listWidget_SKIPLIST->addItem(text);
}

void SettingsSharing::slotAddDirException()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the directory"),
                                                    QDir::home().dirName());
    if (dir.isEmpty())
        return;
    dir = QDir::toNativeSeparators(dir);
    if (!dir.endsWith(QDir::separator()))
        dir += QDir::separator();
    listWidget_SKIPLIST->addItem(dir + "*");
}

void SettingsSharing::slotEditException()
{
    QListWidgetItem *item = listWidget_SKIPLIST->currentItem();
    if (!item)
        return;

    const int row = listWidget_SKIPLIST->row(item);
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Add item"), tr("Enter text:"),
                                               QLineEdit::Normal, item->text(), &ok);
    if (!ok || text.isEmpty())
        return;
    delete item;
    listWidget_SKIPLIST->insertItem(row, text);
    listWidget_SKIPLIST->setCurrentRow(row);
}

void SettingsSharing::slotDeleteException()
{
    delete listWidget_SKIPLIST->currentItem();
}

void SettingsSharing::slotSimpleShareModeChanged()
{
    dirs->setSimpleMode(checkBox_SIMPLE_SHARE_MODE->isChecked());
}

void SettingsSharing::slotContextMenu(const QPoint &pos)
{
    dirs->showSimpleMenu(pos);
}
