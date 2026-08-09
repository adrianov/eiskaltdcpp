/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#pragma once

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QList>
#include <QMenuBar>
#include <QMenu>
#include <QCloseEvent>
#include <QShowEvent>
#include <QTabBar>
#include <QToolBar>
#include <QHash>
#include <QSessionManager>

#include "dcpp/stdinc.h"
#include "dcpp/ConnectionManager.h"
#include "dcpp/DownloadManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/QueueManager.h"
#include "dcpp/TimerManager.h"
#include "dcpp/UploadManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/Download.h"
#include "dcpp/Util.h"

#include "ArenaWidget.h"
#include "HistoryInterface.h"
#include "LineEdit.h"
#include "ShortcutManager.h"
#include "AboutDialog.h"
#include "MainWindowMenuSlots.h"

class FavoriteHubs;
class DownloadQueue;
class ToolBar;
class MultiLineToolBar;
#ifdef USE_JS
class ScriptConsole;
#endif

class HashProgress;
class MainWindowPrivate;

class MainWindow:
        public QMainWindow,
        public  dcpp::Singleton<MainWindow>,
        private dcpp::LogManagerListener,
        private dcpp::TimerManagerListener
{
    Q_OBJECT

friend class dcpp::Singleton<MainWindow>;

    public:

        typedef QList<QAction*> ActionList;

        Q_PROPERTY (QObject* ToolBar READ getToolBar);
        Q_PROPERTY (QMenuBar* MenuBar READ menuBar);

        void beginExit();
        void newHubFrame(QString, QString);
        void browseOwnFiles();
        void startSocket(bool changed);
        void showPortsError(const std::string& port);
        void autoconnect();
        void retranslateUi();
        void reloadSomeSettings();
        void setUnload(bool b);
        ArenaWidget *widgetForRole(ArenaWidget::Role) const;

    Q_SIGNALS:
        void redrawWidgetPanels();
        void coreLogMessage(const QString&);
        void coreUpdateStats(const QMap<QString, QString> &);
        void notifyMessage(int, const QString&, const QString&);

    public Q_SLOTS:
        QObject *getToolBar();
        void addActionOnToolBar(QAction*);
        void remActionFromToolBar(QAction*);
        void toggleMainMenu(bool);
        void slotChatClear();
        void redrawToolPanel();
        void setStatusMessage(QString);
        void show();
        void parseCmdLine(const QStringList &);
        void parseInstanceLine(const QString &);

    protected:
        virtual void closeEvent(QCloseEvent*);
        virtual void showEvent(QShowEvent *);
        virtual void hideEvent(QHideEvent *);
        virtual void changeEvent(QEvent *);
        virtual bool eventFilter(QObject *, QEvent *);

    private Q_SLOTS:
        void mapWidgetOnArena(ArenaWidget*);
        void removeWidget(ArenaWidget *awgt);
        void insertWidget(ArenaWidget *awgt);
        void updated(ArenaWidget *awgt);
        MAINWINDOW_MENU_SLOTS

    private:
        MainWindow (QWidget *parent=nullptr);
        virtual ~MainWindow();

        virtual void on(dcpp::LogManagerListener::Message, time_t t, const std::string&) noexcept;
        virtual void on(dcpp::TimerManagerListener::Second, uint64_t) noexcept;

        void init();
        void loadSettings();
        void saveSettings();
        void getWindowGeometry();
        void initActions();
        void initMenuBar();
        void initStatusBar();
        void initSearchBar();
        void initToolbar();
        void initSideBar();
        void initFavHubMenu();
#if defined(Q_OS_MAC)
        void initDockMenuBar();
#endif
        void toggleSingletonWidget(ArenaWidget *a);
        void updateHashProgressStatus();

        Q_DECLARE_PRIVATE(MainWindow)
        HashProgress *progress_dialog();
        MainWindowPrivate *d_ptr;
};

Q_DECLARE_METATYPE(MainWindow*)
