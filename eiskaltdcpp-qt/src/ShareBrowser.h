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
#include <QMenu>
#include <QItemSelectionModel>
#include <QCloseEvent>
#include <QVector>

#include "ArenaWidget.h"
#include "WulforUtil.h"
#include "ui_UIShareBrowser.h"
#include "filebrowser/FileBrowserFilterProxy.h"
#include "filebrowser/ListFilterProxy.h"
#include "sharebrowser/ShareBrowserMenu.h"
#include "sharebrowser/ShareFolderList.h"
#include "SearchFileTypes.h"

#include "dcpp/stdinc.h"
#include "dcpp/User.h"
#include "dcpp/DirectoryListing.h"

class FileBrowserModel;
class FileBrowserItem;
class MediaEnrichQueue;
class QModelIndex;

class ShareBrowser : public QWidget,
                     public ArenaWidget,
                     private Ui::UIShareBrowser
{
    Q_OBJECT
    Q_INTERFACES(ArenaWidget)

public:
    ShareBrowser(dcpp::UserPtr, const QString &, const QString &);
    ~ShareBrowser() override;

    QString  getArenaTitle() override;
    QString  getArenaShortTitle() override;
    QWidget *getWidget() override;
    QMenu   *getMenu() override;
    const QPixmap &getPixmap() override { return WICON(AppIcons::eiOWN_FILELIST); }
    ArenaWidget::Role role() const override { return ArenaWidget::ShareBrowser; }

protected:
    void closeEvent(QCloseEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
    /** Cap width so the filter toolbar cannot limit the main side dock. */
    QSize minimumSizeHint() const override;

Q_SIGNALS:
    void die(const QString &msg);

private Q_SLOTS:
    void init();
    void slotApplyFilters();
    void slotClearFilters();
    void flushViewFilters();
    void slotFlatToggled(bool);
    void slotRightPaneClicked(const QModelIndex&);
    void slotRightPaneSelChanged(const QItemSelection&, const QItemSelection&);
    void slotLeftPaneSelChanged(const QItemSelection&, const QItemSelection&);
    void slotCustomContextMenu(const QPoint&);
    void slotHeaderMenu();
    void slotLayoutUpdated();
    void slotSettingsChanged(const QString&, const QString&);
    void slotButtonBack();
    void slotButtonForward();
    void slotButtonUp();
    void slotClose();
    void slotAddToFavorites();
    void slotDie(const QString &msg);
    /** Apply packed TTH→media map from MediaEnrichQueue. */
    void applyMediaEnrich(const QVariant &packed);

private:
    void continueInit();
    void selectLeftFolder(FileBrowserItem *);

    void load();
    void save();

    void buildList();
    void initModels();

    void download(dcpp::DirectoryListing::Directory*, const QString &);
    void download(dcpp::DirectoryListing::File*, const QString &);
    void contextMoreActions(ShareBrowserMenu::Action act, const QModelIndexList &list);
    void contextUserActions(ShareBrowserMenu::Action act, const QModelIndexList &list);
    void deleteOwnItems(const QModelIndexList &list);

    void changeRoot(dcpp::DirectoryListing::Directory*);
    void changeRootFlat(dcpp::DirectoryListing::Directory*);
    void loadRoot(dcpp::DirectoryListing::Directory *root, bool flat);
    void applyFlatMode(bool on);
    void restoreSplitterSizes();
    void updateUpButton();
    void goToFlatItem(FileBrowserItem *item);
    void applyOptionalColumns();
    void reloadRightPane(dcpp::DirectoryListing::Directory *dir);
    dcpp::DirectoryListing::Directory *currentDir();

    void readSizeFilter(quint64 &size, int &mode) const;
    void applyViewFiltersNow();
    QString totalStatusText() const;

    QModelIndex treeMapToSource(const QModelIndex &index) const;
    QModelIndex treeMapFromSource(const QModelIndex &index) const;

    void goUp(QTreeView *);
    void goDown(QTreeView *);

    struct SelPair
    {
        dcpp::DirectoryListing::Directory *dir;
        QString path_tesxt;
        QModelIndex index;
    };

    QMenu *arena_menu = nullptr;

    ListFilterProxy *proxy = nullptr;
    FileBrowserFilterProxy *tree_proxy = nullptr;
    bool viewFilterPending = false;
    bool flatMode = false;
    bool splitReady = false;

    QVector<SelPair>::iterator pathHistory_iter;
    QVector<SelPair> pathHistory;

    QString nick;
    QString file;
    QString title;
    QString jump_to;
    dcpp::DirectoryListing listing;
    dcpp::UserPtr user;

    FileBrowserModel *tree_model = nullptr;
    FileBrowserModel *list_model = nullptr;
    FileBrowserItem  *tree_root = nullptr;
    FileBrowserItem  *list_root = nullptr;
    ShareFolderList  *folderList = nullptr;
    MediaEnrichQueue *mediaEnrich = nullptr;
    SearchFileTypes::ListingTypes listingTypes_;
};
