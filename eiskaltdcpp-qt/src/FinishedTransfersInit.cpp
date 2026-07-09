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
    QObject::connect(comboBox, SIGNAL(activated(int)), this, SLOT(slotTypeChanged(int)));
    QObject::connect(pushButton, SIGNAL(clicked()), this, SLOT(slotClear()));
    QObject::connect(treeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotItemDoubleClicked(const QModelIndex &)));
    QObject::connect(treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));
    QObject::connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
    QObject::connect(checkBox_FULL, SIGNAL(toggled(bool)), this, SLOT(slotSwitchOnlyFull(bool)));
    QObject::connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), this, SLOT(slotFilterText(QString)));

    if (isUpload) {
        FinishedTransfers::slotSwitchOnlyFull(false);
    } else {
        checkBox_FULL->hide();
        FinishedTransfers::slotSwitchOnlyFull(true);
    }
    FinishedTransfers::slotTypeChanged(0);

    ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
}

template <bool isUpload>
FinishedTransfers<isUpload>::~FinishedTransfers(){
    QString key = (comboBox->currentIndex() == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
    WVSET(key, treeView->header()->saveState());

    FinishedManager::getInstance()->removeListener(this);

    model->clearModel();

#ifdef USE_QT_SQLITE
    // Close then drop the registry entry before QApplication teardown
    // (QtSql post-routines abort in sqlite if connections linger).
    const QString conn = db.connectionName();
    db.close();
    db = QSqlDatabase();
    if (!conn.isEmpty())
        QSqlDatabase::removeDatabase(conn);
#endif

    delete proxy;
    delete model;
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotTypeChanged(int index) {
    QString from_key = (index == 0)? WS_FTRANSFERS_USERS_STATE : WS_FTRANSFERS_FILES_STATE;
    QString to_key = (index == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
    QByteArray old_state = treeView->header()->saveState();

    if (sender() == comboBox)
        WVSET(from_key, old_state);

    QByteArray state = WVGET(to_key, QByteArray()).toByteArray();
    WulforUtil::restoreTreeHeader(treeView->header(), state);
    treeView->setSortingEnabled(true);

    model->switchViewType(static_cast<FinishedTransfersModel::ViewType>(index));

    if (state.isEmpty())
        treeView->sortByColumn(COLUMN_FINISHED_TIME, Qt::AscendingOrder);
    else {
        const int sortCol = treeView->header()->sortIndicatorSection();
        if (sortCol >= 0)
            treeView->sortByColumn(sortCol, treeView->header()->sortIndicatorOrder());
    }
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotClear() {
    model->clearModel();

    try {
        FinishedManager::getInstance()->removeAll(isUpload);
    }
    catch (const std::exception&){}

#ifdef USE_QT_SQLITE
    if (!db_opened)
        return;

    QSqlQuery q(db);
    q.exec("DROP TABLE files;");
    q.exec("DROP TABLE users;");
#endif
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotHeaderMenu() {
    WulforUtil::headerMenu(treeView);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotSwitchOnlyFull(bool checked) {
    proxy->setFullOnly(checked);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotFilterText(const QString &text) {
    proxy->setTextFilter(text);
}

template <bool isUpload>
void FinishedTransfers<isUpload>::slotSettingsChanged(const QString &key, const QString &) {
    if (key == WS_TRANSLATION_FILE) {
        retranslateUi(this);
        lineEdit_FILTER->setPlaceholderText(tr("Filter"));
        lineEdit_FILTER->setToolTip(tr("Filter by name, path, or user"));
    }
}

template FinishedTransfers<true>::FinishedTransfers(QWidget*);
template FinishedTransfers<false>::FinishedTransfers(QWidget*);
template FinishedTransfers<true>::~FinishedTransfers();
template FinishedTransfers<false>::~FinishedTransfers();
template void FinishedTransfers<true>::slotTypeChanged(int);
template void FinishedTransfers<false>::slotTypeChanged(int);
template void FinishedTransfers<true>::slotClear();
template void FinishedTransfers<false>::slotClear();
template void FinishedTransfers<true>::slotHeaderMenu();
template void FinishedTransfers<false>::slotHeaderMenu();
template void FinishedTransfers<true>::slotSwitchOnlyFull(bool);
template void FinishedTransfers<false>::slotSwitchOnlyFull(bool);
template void FinishedTransfers<true>::slotFilterText(const QString&);
template void FinishedTransfers<false>::slotFilterText(const QString&);
template void FinishedTransfers<true>::slotSettingsChanged(const QString&, const QString&);
template void FinishedTransfers<false>::slotSettingsChanged(const QString&, const QString&);
