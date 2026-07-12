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
#include <QModelIndex>
#include <QMap>
#include <QList>
#include <QMenu>
#include <QCloseEvent>
#include <QMetaType>

#include "ui_UISearchFrame.h"
#include "ArenaWidget.h"
#include "SearchStringListModel.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchResult.h"
#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ClientManagerListener.h"
#include "dcpp/QueueManagerListener.h"
#include "dcpp/Singleton.h"

using namespace dcpp;

class SearchItem;
class SearchFramePrivate;

class SearchFrame : public QWidget,
                    public ArenaWidget,
                    private Ui::SearchFrame,
                    private SearchManagerListener,
                    private ClientManagerListener,
                    private QueueManagerListener
{
    Q_OBJECT
    Q_INTERFACES(ArenaWidget)

    typedef QVariantMap VarMap;

    class Menu : public dcpp::Singleton<Menu> {
        friend class dcpp::Singleton<Menu>;
    public:
        enum Action {
            Download=0,
            DownloadTo,
            DownloadWholeDir,
            DownloadWholeDirTo,
            OpenFile,
            OpenDirectory,
            SearchTTH,
            CopyFileName,
            Magnet,
            MagnetWeb,
            MagnetInfo,
            Browse,
            MatchQueue,
            SendPM,
            AddToFav,
            GrantExtraSlot,
            RemoveFromQueue,
            Remove,
            UserCommands,
            Blacklist,
            AddToBlacklist,
            None
        };

        Action exec(const QStringList &, bool canOpenLocal = false);
        QMenu *buildUserCmdMenu(QList<QString> hubs);
        QString getDownloadToPath() { return downToPath; }
        int getCommandId() { return uc_cmd_id; }
        void addTempPath(const QString &path);

    private:
        Menu();
        virtual ~Menu();

        QMap<QAction*, Action> actions;
        QList<QAction*> action_list;

        QString downToPath;

        int uc_cmd_id;

        QMenu *menu;
        QMenu *magnet_menu;
        QMenu *down_to;
        QMenu *down_wh_to;
        QMenu *black_list_menu;
    };

public:
    enum AlreadySharedAction{
        None=0,
        Filter,
        Highlight
    };

    SearchFrame(QWidget* = nullptr);
    virtual ~SearchFrame();

    QWidget *getWidget();
    QString  getArenaTitle();
    QString  getArenaShortTitle();
    QMenu   *getMenu();
    const QPixmap &getPixmap();
    ArenaWidget::Role role() const { return ArenaWidget::Search; }

    void requestFilter() { slotFilter(); }
    void requestFocus() { lineEdit_SEARCHSTR->setFocus(); }

public Q_SLOTS:
    void searchAlternates(const QString &);
    void searchFile(const QString &);
    void fastSearch(const QString &, bool);
    /** Local ShareIndex hits (queued from a worker thread). */
    void addResultsPacked(const QVariant &packed);

protected:
    virtual void closeEvent(QCloseEvent*);
    virtual bool eventFilter(QObject *, QEvent *);

Q_SIGNALS:
    void coreSR(const VarMap&);
    void coreClientConnected(const QString &info);
    void coreClientUpdated(const QString &info);
    void coreClientDisconnected(const QString &info);
    void coreDownloadFinished(const QString &tth);

private Q_SLOTS:
    void slotFilter();
    void slotClear();
    void slotTimer();
    void slotResultDoubleClicked(const QModelIndex&);
    void slotContextMenu(const QPoint&);
    void slotHeaderMenu(const QPoint&);
    void slotToggleSidePanel();
    void slotStartSearch();
    void slotStopSearch();
    void slotChangeProxyColumn(int);
    void slotClose();
    void slotSettingsChanged(const QString &key, const QString &value);
    void onHubAdded(const QString &info);
    void onHubChanged(const QString &info);
    void onHubRemoved(const QString &info);
    void addResult(const VarMap &map);
    void addResults(const QList<VarMap> &maps);
    void queueResult(const VarMap &map);
    void flushResults();
    void setIndexStats(const QString &text);
    void slotDownloadFinished(const QString &tth);
    void slotLocalRefreshAll();

private:
    void init();
    void load();
    void save();
    void getParams(VarMap&, const dcpp::SearchResultPtr&);
    bool getDownloadParams(VarMap&, SearchItem*);
    bool getWholeDirParams(VarMap&, SearchItem*);
    void rememberSearch(const QString &s);
    void download(const VarMap&);
    bool contextDownloads(Menu::Action act, const QModelIndexList &list);
    bool contextMoreActions(Menu::Action act, const QModelIndexList &list);
    bool contextUserActions(Menu::Action act, const QModelIndexList &list);
    void getFileList(const VarMap&, bool = false);
    void addToFav(const QString&);
    void grant(const VarMap&);
    void removeSource(const VarMap&);
    virtual void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept;
    virtual void on(ClientConnected, Client* c) noexcept;
    virtual void on(ClientUpdated, Client* c) noexcept;
    virtual void on(ClientDisconnected, Client* c) noexcept;
    virtual void on(QueueManagerListener::Finished, QueueItem*, const string&, int64_t) noexcept;
    virtual void on(QueueManagerListener::FileMoved, const string&) noexcept;

    Q_DECLARE_PRIVATE (SearchFrame)
    SearchFramePrivate* d_ptr;
};

Q_DECLARE_METATYPE(SearchFrame*)
