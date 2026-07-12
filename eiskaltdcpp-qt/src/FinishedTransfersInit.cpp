/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "FinishedTransfers.h"
#include "FinishedTransfersProxy.h"
#include "SearchFileTypes.h"
#include "WulforSettings.h"

template <bool isUpload>
FinishedTransfers<isUpload>::FinishedTransfers(QWidget *parent) :
    FinishedTransferProxy(parent), db_opened(false), diskPruned(false)
{
    setupUi(this);

    model = new FinishedTransfersModel();

    proxy = new FinishedTransferProxyModel(!isUpload, !isUpload);
    if (!isUpload) {
        model->setHideFileLists(true);
        model->setRequireFullFile(true);
    }
    proxy->setDynamicSortFilter(true);
    proxy->setSourceModel(model);

    treeView->setModel(proxy);

#ifdef USE_QT_SQLITE
    db = QSqlDatabase::addDatabase("QSQLITE", (isUpload? "FinishedUploads" : "FinishedDownloads"));
    db_file = _q(Util::getPath(Util::PATH_USER_CONFIG)) + (isUpload? "FinishedUploads.sqlite" : "FinishedDownloads.sqlite");

    db.setDatabaseName(db_file);
    db_opened = db.open();

    if (db_opened){
        QSqlQuery q(db);
        q.exec("CREATE TABLE IF NOT EXISTS files (FNAME TEXT PRIMARY KEY, "
               "TIME TEXT, PATH TEXT, USERS TEXT, TR TEXT, SPEED TEXT, CRC32 INTEGER, TARGET TEXT, ELAP TEXT, FULL INTEGER);");

        q.exec("CREATE TABLE IF NOT EXISTS users (NICK TEXT PRIMARY KEY, "
               "TIME TEXT, FILES TEXT, TR TEXT, SPEED TEXT, CID TEXT, ELAP TEXT, FULL INTEGER);");

        q.exec("VACUUM;");
    }
#endif

    loadList();

    FinishedManager::getInstance()->addListener(this);

    setUnload(false);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(this, SIGNAL(coreAddedFile(VarMap)),   model, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreAddedUser(VarMap)),   model, SLOT(addUser(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreUpdatedFile(VarMap)), model, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreUpdatedUser(VarMap)), model, SLOT(addUser(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreRemovedFile(QString)), model, SLOT(remFile(QString)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreRemovedUser(QString)), model, SLOT(remUser(QString)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreBeginBulkLoad()), model, SLOT(beginBulkLoad()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreEndBulkLoad()), model, SLOT(endBulkLoad()), Qt::QueuedConnection);

    QObject::connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    SearchFileTypes::fillCombo(comboBox_FILETYPES, false);

    QObject::connect(comboBox, SIGNAL(activated(int)), this, SLOT(slotTypeChanged(int)));
    QObject::connect(pushButton, SIGNAL(clicked()), this, SLOT(slotClear()));
    QObject::connect(treeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotItemDoubleClicked(const QModelIndex &)));
    QObject::connect(treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));
    QObject::connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
    QObject::connect(checkBox_FULL, SIGNAL(toggled(bool)), this, SLOT(slotSwitchOnlyFull(bool)));
    QObject::connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), this, SLOT(slotFilterText(QString)));
    QObject::connect(comboBox_FILETYPES, SIGNAL(currentIndexChanged(int)), this, SLOT(slotFileTypeChanged(int)));

    if (isUpload) {
        FinishedTransfers::slotSwitchOnlyFull(false);
    } else {
        checkBox_FULL->hide();
        FinishedTransfers::slotSwitchOnlyFull(true);
    }
    FinishedTransfers::slotTypeChanged(0);
    FinishedTransfers::slotFileTypeChanged(comboBox_FILETYPES->currentIndex());

    ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
}

template FinishedTransfers<true>::FinishedTransfers(QWidget*);
template FinishedTransfers<false>::FinishedTransfers(QWidget*);
