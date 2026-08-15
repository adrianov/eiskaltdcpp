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
#include <QList>
#include <QMenu>
#include <QCloseEvent>
#include <QShowEvent>
#include <QMetaType>

#include "ui_UISearchFrame.h"
#include "ArenaWidget.h"
#include "SearchStringListModel.h"
#include "search/SearchResultsMenu.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchResult.h"
#include "dcpp/SearchManager.h"
#include "dcpp/ClientManagerListener.h"
#include "dcpp/QueueManagerListener.h"

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
    using Menu = SearchResultsMenu;

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
    bool titleBold() const;
    QMenu   *getMenu();
    const QPixmap &getPixmap();
    ArenaWidget::Role role() const { return ArenaWidget::Search; }

    void requestFocus() { lineEdit_SEARCHSTR->setFocus(); }
    /** Ctrl+F: focus the search box (live substring filter). */
    void requestFilter() { requestFocus(); }

public Q_SLOTS:
    void searchAlternates(const QString &);
    void searchFile(const QString &);
    void fastSearch(const QString &, bool);
    /** Local ShareIndex hits (queued from a worker thread). */
    void addResultsPacked(const QVariant &packed);

protected:
    virtual void closeEvent(QCloseEvent*);
    void showEvent(QShowEvent*);

Q_SIGNALS:
    void coreSR(const VarMap&);
    void coreClientConnected(const QString &info);
    void coreClientUpdated(const QString &info);
    void coreClientDisconnected(const QString &info);
    void coreDownloadFinished(const QString &tth);

private Q_SLOTS:
    void slotClear();
    void slotTimer();
    void slotResultDoubleClicked(const QModelIndex&);
    void slotContextMenu(const QPoint&);
    void slotHeaderMenu(const QPoint&);
    void slotToggleSidePanel();
    void slotStartSearch();
    void slotStopSearch();
    /** Queue live view filter apply (coalesces typing bursts). */
    void slotApplyViewFilters();
    /** Apply pending view filters once. */
    void flushViewFilters();
    void persistFileType();
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
    /** Apply packed TTH→media map from MediaEnrichQueue. */
    void applyMediaEnrich(const QVariant &packed);

private:
    void init();
    void load();
    void save();
    void getParams(VarMap&, const dcpp::SearchResultPtr&);
    bool getDownloadParams(VarMap&, SearchItem*);
    bool getWholeDirParams(VarMap&, SearchItem*);
    void rememberSearch(const QString &s);
    void download(const VarMap&);
    /** Hide TTH always; hide Bitrate/Resolution/Video/Audio when unused. */
    void applyOptionalColumns();
    /** Second ShareIndex media pass after the search progress window ends. */
    void requeueMissingMedia();
    bool contextDownloads(Menu::Action act, const QModelIndexList &list);
    bool contextLocalOpen(Menu::Action act, const QModelIndexList &list);
    bool contextCopyClip(Menu::Action act, const QModelIndexList &list);
    bool contextMoreActions(Menu::Action act, const QModelIndexList &list);
    bool contextUserActions(Menu::Action act, const QModelIndexList &list);
    void getFileList(const VarMap&, bool = false);
    void addToFav(const QString&);
    void grant(const VarMap&);
    void removeSource(const VarMap&);
    /** Read size widgets into bytes + SizeModes (DONTCARE when empty/zero). */
    void readSizeFilter(quint64 &size, int &mode) const;
    /** Apply search-box terms, size, and type to the proxy now. */
    void applyViewFiltersNow();
    /** Drop queued/shown hits and the unseen-results mark. */
    void resetResultState();
    /** Bold the sidebar entry until this search is shown again. */
    void noteUnseenResults();
    void clearUnseenResults();
    virtual void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept;
    virtual void on(ClientConnected, Client* c) noexcept;
    virtual void on(ClientUpdated, Client* c) noexcept;
    virtual void on(ClientDisconnected, Client* c) noexcept;
    virtual void on(QueueManagerListener::Added, QueueItem*) noexcept;
    virtual void on(QueueManagerListener::Removed, QueueItem*) noexcept;
    virtual void on(QueueManagerListener::Finished, QueueItem*, const string&, int64_t) noexcept;
    virtual void on(QueueManagerListener::FileMoved, const string&) noexcept;

    Q_DECLARE_PRIVATE (SearchFrame)
    SearchFramePrivate* d_ptr;
};

Q_DECLARE_METATYPE(SearchFrame*)
