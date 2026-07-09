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
#include <QHeaderView>

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
#include "FinishedTransfersProxy.h"
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
    void coreBeginBulkLoad();
    void coreEndBulkLoad();

public Q_SLOTS:
    virtual void slotTypeChanged(int) = 0;
    virtual void slotClear() = 0;
    virtual void slotItemDoubleClicked(const QModelIndex &) = 0;
    virtual void slotContextMenu() = 0;
    virtual void slotHeaderMenu() = 0;
    virtual void slotSwitchOnlyFull(bool) = 0;
    virtual void slotFilterText(const QString &) = 0;
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
    FinishedTransfers(QWidget *parent = nullptr);
    ~FinishedTransfers();

    void loadList();
    void loadListFromDB();
    void getParams(const FinishedFileItemPtr& item, const string& file, VarMap &params);
    void getParams(const FinishedUserItemPtr& item, const UserPtr& user, VarMap &params);
    void removeFileFromDB(const QString &target);
    void pruneMissingFiles();

    void slotTypeChanged(int index) override;
    void slotClear() override;
    void openFile(QString file);
    void slotItemDoubleClicked(const QModelIndex &proxyIndex) override;
    void slotContextMenu() override;
    void slotHeaderMenu() override;
    void slotSwitchOnlyFull(bool checked) override;
    void slotFilterText(const QString &text) override;
    void slotSettingsChanged(const QString &key, const QString &) override;

    void on(FinishedManagerListener::AddedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept override;
    void on(FinishedManagerListener::AddedUser, bool upload, const dcpp::HintedUser &user, const FinishedUserItemPtr &item) noexcept override;
    void on(FinishedManagerListener::UpdatedFile, bool upload, const std::string &file, const FinishedFileItemPtr &item) noexcept override;
    void on(FinishedManagerListener::RemovedFile, bool upload, const std::string &file) noexcept override;
    void on(FinishedManagerListener::UpdatedUser, bool upload, const dcpp::HintedUser &user) noexcept override;
    void on(FinishedManagerListener::RemovedUser, bool upload, const dcpp::HintedUser &user) noexcept override;

    FinishedTransferProxyModel *proxy;
    FinishedTransfersModel *model;

    static bool isFileListPath(const std::string &file) {
        return isFinishedFileList(file);
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
