/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QWidget>
#include <QEvent>
#include <QCloseEvent>
#include <QShowEvent>
#include <QDir>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QDesktopServices>
#include <QHeaderView>
#include <QUrl>

#ifdef USE_QT_SQLITE
#include <QtSql>
#endif

#include "dcpp/stdinc.h"
#include "dcpp/FinishedManager.h"
#include "dcpp/Util.h"
#include "dcpp/FinishedItem.h"
#include "dcpp/User.h"
#include "dcpp/Singleton.h"

#include "ui_UIFinishedTransfers.h"
#include "ArenaWidget.h"
#include "WulforUtil.h"
#include "FinishedTransfersModel.h"
#include "ShareBrowser.h"

using namespace dcpp;

class FinishedTransferProxy: public QWidget{
Q_OBJECT
typedef QVariantMap VarMap;
public:
    FinishedTransferProxy(QWidget *parent):QWidget(parent){}
    ~FinishedTransferProxy(){}

    QString uploadTitle();
    QString downloadTitle();

Q_SIGNALS:
    void coreAddedFile(const VarMap&);
    void coreAddedUser(const VarMap&);
    void coreUpdatedFile(const VarMap&);
    void coreUpdatedUser(const VarMap&);
    void coreRemovedFile(const QString&);
    void coreRemovedUser(const QString&);

public Q_SLOTS:
    virtual void slotTypeChanged(int) = 0;
    virtual void slotClear() = 0;
    virtual void slotItemDoubleClicked(const QModelIndex &) = 0;
    virtual void slotContextMenu() = 0;
    virtual void slotHeaderMenu() = 0;
    virtual void slotSwitchOnlyFull(bool) = 0;
    virtual void slotSettingsChanged(const QString &key, const QString &) = 0;
};

template <bool isUpload>
class FinishedTransfers :
        public dcpp::FinishedManagerListener,
        private Ui::UIFinishedTransfers,
        public dcpp::Singleton< FinishedTransfers<isUpload> >,
        public ArenaWidget,
        public FinishedTransferProxy
{
Q_INTERFACES(ArenaWidget)

typedef QVariantMap VarMap;
friend class dcpp::Singleton< FinishedTransfers<isUpload> >;

public:
    QWidget *getWidget() override { return this;}
    QString getArenaTitle() override { return (isUpload? uploadTitle() : downloadTitle()); }
    QString getArenaShortTitle() override { return getArenaTitle(); }
    QMenu *getMenu() override { return nullptr; }
    ArenaWidget::Role role() const override;

    const QPixmap &getPixmap() override {
        if (isUpload)
            return WICON(WulforUtil::eiUPLIST);
        else
            return WICON(WulforUtil::eiDOWNLIST);
    }

protected:
    void closeEvent(QCloseEvent *e) override {
        isUnload()? e->accept() : e->ignore();
    }

    void showEvent(QShowEvent *e) override {
        QWidget::showEvent(e);
        if (!isUpload && !diskPruned) {
            diskPruned = true;
            pruneMissingFiles();
        }
    }

private:
    FinishedTransfers(QWidget *parent = nullptr) :
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

        QObject::connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString,QString)), this, SLOT(slotSettingsChanged(QString,QString)));
        QObject::connect(comboBox, SIGNAL(activated(int)), this, SLOT(slotTypeChanged(int)));
        QObject::connect(pushButton, SIGNAL(clicked()), this, SLOT(slotClear()));
        QObject::connect(treeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotItemDoubleClicked(const QModelIndex &)));
        QObject::connect(treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenu()));
        QObject::connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu()));
        QObject::connect(checkBox_FULL, SIGNAL(toggled(bool)), this, SLOT(slotSwitchOnlyFull(bool)));

        if (isUpload) {
            FinishedTransfers::slotSwitchOnlyFull(false);
        } else {
            checkBox_FULL->hide();
            FinishedTransfers::slotSwitchOnlyFull(true);
        }
        FinishedTransfers::slotTypeChanged(0);

        ArenaWidget::setState( ArenaWidget::Flags(ArenaWidget::state() | ArenaWidget::Singleton | ArenaWidget::Hidden) );
    }

    ~FinishedTransfers(){
        QString key = (comboBox->currentIndex() == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
        WVSET(key, treeView->header()->saveState());

        FinishedManager::getInstance()->removeListener(this);

        model->clearModel();

#ifdef USE_QT_SQLITE
        db.close();
#endif

        delete proxy;
        delete model;
    }

    void loadList();
    void loadListFromDB();
    void getParams(const FinishedFileItemPtr& item, const string& file, VarMap &params);
    void getParams(const FinishedUserItemPtr& item, const UserPtr& user, VarMap &params);
    void removeFileFromDB(const QString &target);
    void pruneMissingFiles();

    void slotTypeChanged(int index) override {
        QString from_key = (index == 0)? WS_FTRANSFERS_USERS_STATE : WS_FTRANSFERS_FILES_STATE;
        QString to_key = (index == 0)? WS_FTRANSFERS_FILES_STATE : WS_FTRANSFERS_USERS_STATE;
        QByteArray old_state = treeView->header()->saveState();

        if (sender() == comboBox)
            WVSET(from_key, old_state);

        QByteArray state = WVGET(to_key, QByteArray()).toByteArray();
        WulforUtil::restoreTreeHeader(treeView->header(), state);
        treeView->setSortingEnabled(true);

        model->switchViewType(static_cast<FinishedTransfersModel::ViewType>(index));

        if (index == FinishedTransfersModel::FileView)
            proxy->setFilterKeyColumn(COLUMN_FINISHED_FULL);
        else
            proxy->setFilterKeyColumn(COLUMN_FINISHED_CRC32);

        if (state.isEmpty())
            treeView->sortByColumn(COLUMN_FINISHED_TIME, Qt::AscendingOrder);
    }

    void slotClear() override {
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

    void openFile(QString file){
        if (!file.startsWith(QChar('/')))
            file.prepend("/");

        int sep = file.lastIndexOf(QDir::separator());
        QString name = file.right(sep);
        QString path = file.left(sep);

        QDir test(path);
        if (!test.exists(file)){
            QStringList files = test.entryList(QStringList("*"+name+"*"), QDir::Files, QDir::Name);

            if (files.size() > 0)
                file = path + QDir::separator() + files.first();
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
    }

    void slotItemDoubleClicked(const QModelIndex &proxyIndex) override {
        Q_UNUSED(proxyIndex);

        if (comboBox->currentIndex())
            return;

        QItemSelectionModel *s_model = treeView->selectionModel();
        QModelIndexList p_indexes = s_model->selectedRows(0);
        QModelIndexList indexes;

        for (const auto &i : p_indexes)
            indexes.push_back(proxy->mapToSource(i));

        if (indexes.size() < 1)
            return;

        QStringList files;
        FinishedTransfersItem *item = nullptr;
        QString file;
        bool full;

        for (const auto &i : indexes){
            item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
            file = item->data(COLUMN_FINISHED_TARGET).toString();
            full = item->data(COLUMN_FINISHED_FULL).toBool();

            if (!file.isEmpty() && full)
                files.push_back(file);
        }

        for (const auto &f : files)
            openFile(f);
    }

    void slotContextMenu() override {
        static WulforUtil *WU = WulforUtil::getInstance();

        QItemSelectionModel *s_model = treeView->selectionModel();
        QModelIndexList p_indexes = s_model->selectedRows(0);
        QModelIndexList indexes;

        for (const auto &i : p_indexes)
            indexes.push_back(proxy->mapToSource(i));

        if (indexes.size() < 1)
            return;

        QStringList files;

        if (comboBox->currentIndex() == 0){
            FinishedTransfersItem *item = nullptr;
            QString file;

            for (const auto &i : indexes){
                item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
                file = item->data(COLUMN_FINISHED_TARGET).toString();

                if (!file.isEmpty())
                    files.push_back(file);
            }
        }
        else {
            FinishedTransfersItem *item = nullptr;
            QString file_list;

            for (const auto &i : indexes){
                item = reinterpret_cast<FinishedTransfersItem*>(i.internalPointer());
                file_list = item->data(COLUMN_FINISHED_PATH).toString();

                if (!file_list.isEmpty()){
                    files.append(file_list.split("; ", WULFOR_SKIP_EMPTY));
                }

            }
        }

        QMenu *m = new QMenu();
        QAction *open_f   = new QAction(tr("Open file"), m);
        QAction *open_dir = new QAction(WU->getPixmap(WulforUtil::eiFOLDER_BLUE), tr("Open directory"), m);

        m->addAction(open_f);
        m->addAction(open_dir);

        QAction *ret = m->exec(QCursor::pos());

        delete m;

        if (ret == open_f){
            for (const auto &f : files)
                openFile(f);
        }
        else if (ret == open_dir){
            for (const auto &f : files)
                WulforUtil::revealPath(f);
        }

    }

    void slotHeaderMenu() override {
        WulforUtil::headerMenu(treeView);
    }

    void slotSwitchOnlyFull(bool checked) override {
        proxy->setFilterFixedString((checked? "1" : ""));
    }

    void slotSettingsChanged(const QString &key, const QString &) override {
        if (key == WS_TRANSLATION_FILE)
            retranslateUi(this);
    }

    void on(FinishedManagerListener::AddedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept override {
        if (isUpload != upload)
            return;

        if (!showDownload(file, item)) {
            if (!isUpload && isFileListPath(file)) {
                try {
                    FinishedManager::getInstance()->remove(false, file);
                } catch (const std::exception&) {}
            }
            return;
        }

        VarMap params;

        getParams(item, file, params);

        emit coreAddedFile(params);
    }

    void on(FinishedManagerListener::AddedUser, bool upload, const dcpp::HintedUser &user, const FinishedUserItemPtr &item) noexcept override {
        if (isUpload == upload){
            VarMap params;

            getParams(item, user, params);

            emit coreAddedUser(params);
        }
    }

    void on(FinishedManagerListener::UpdatedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept override {
        if (isUpload == upload && showDownload(file, item)){
            VarMap params;

            getParams(item, file, params);

            emit coreUpdatedFile(params);
        }
    }

    void on(FinishedManagerListener::RemovedFile, bool upload, const std::string &file) noexcept override {
        if (isUpload == upload){
            removeFileFromDB(_q(file));
            emit coreRemovedFile(_q(file));
        }
    }

    void on(FinishedManagerListener::UpdatedUser, bool upload, const dcpp::HintedUser &user) noexcept override {
        if (isUpload == upload){
            const FinishedManager::MapByUser &umap = FinishedManager::getInstance()->getMapByUser(isUpload);
            auto userit = umap.find(user);
            if (userit == umap.end())
                return;

            const FinishedUserItemPtr &item = userit->second;

            VarMap params;

            getParams(item, user, params);

            emit coreUpdatedUser(params);
        }
    }

    void on(FinishedManagerListener::RemovedUser, bool upload, const dcpp::HintedUser &user) noexcept override {
        if (isUpload == upload){
            emit coreRemovedUser(_q(user.user->getCID().toBase32()));
        }
    }

    FinishedTransferProxyModel *proxy;
    FinishedTransfersModel *model;

    static bool isFileListPath(const std::string &file) {
        if(file.empty())
            return false;

        const string listPath = Util::getListPath();
        if(!listPath.empty() && file.size() >= listPath.size() &&
           Util::stricmp(file.substr(0, listPath.size()).c_str(), listPath.c_str()) == 0)
            return true;

        if(file.size() >= 4 && Util::stricmp(file.substr(file.size() - 4).c_str(), ".xml") == 0)
            return true;
        if(file.size() >= 7 && Util::stricmp(file.substr(file.size() - 7).c_str(), ".xml.bz2") == 0)
            return true;
        return Util::stricmp(Util::getFileExt(file).c_str(), ".DcLst") == 0;
    }

    bool showDownload(const std::string &file, const FinishedFileItemPtr &item) const {
        if (isUpload)
            return true;
        return item->isFull() && !isFileListPath(file);
    }

    bool showDownloadParams(const VarMap &params) const {
        if (isUpload)
            return true;
        if (!params["FULL"].toBool())
            return false;

        const string target = _tq(params["TARGET"].toString());
        const string path = _tq(params["PATH"].toString() + params["FNAME"].toString());
        return !isFileListPath(target) && !isFileListPath(path);
    }

#ifdef USE_QT_SQLITE
    QSqlDatabase db;
    QString db_file;
#endif
    bool db_opened;
    bool diskPruned;
};

template <>
inline ArenaWidget::Role FinishedTransfers<false>::role() const { return ArenaWidget::FinishedDownloads; }

template <>
inline ArenaWidget::Role FinishedTransfers<true>::role() const { return ArenaWidget::FinishedUploads; }

typedef FinishedTransfers<true>  FinishedUploads;
typedef FinishedTransfers<false> FinishedDownloads;
