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
    FinishedTransferProxy(parent)
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

    openDatabase();

    setUnload(false);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    // Persist is done in the listener (thread-safe SQLite). Signals only update the model.
    QObject::connect(this, SIGNAL(coreAddedFile(VarMap)),   model, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreAddedUser(VarMap)),   model, SLOT(addUser(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreUpdatedFile(VarMap)), model, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreUpdatedUser(VarMap)), model, SLOT(addUser(VarMap)), Qt::QueuedConnection);
    // DB load updates the model only — never re-persist (avoids clobbering live writes).
    QObject::connect(this, SIGNAL(coreLoadedFile(VarMap)),  model, SLOT(addFile(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreLoadedUser(VarMap)),  model, SLOT(addUser(VarMap)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreRemovedFile(QString)), model, SLOT(remFile(QString)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreRemovedUser(QString)), model, SLOT(remUser(QString)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreBeginBulkLoad()), model, SLOT(beginBulkLoad()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(coreEndBulkLoad()), model, SLOT(endBulkLoad()), Qt::QueuedConnection);

    QObject::connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    SearchFileTypes::fillCombo(comboBox_FILETYPES, false);

    // After signal wiring so async DB load and live events are not dropped.
    FinishedManager::getInstance()->addListener(this);
    loadList();

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
