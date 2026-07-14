/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShareBrowser.h"
#include "WulforUtil.h"
#include "FileBrowserModel.h"
#include "MainWindow.h"
#include "ArenaWidgetManager.h"

#include "dcpp/SettingsManager.h"
#include "dcpp/ClientManager.h"
#include <dcpp/ADLSearch.h>

#if (HAVE_MALLOC_TRIM)
#include <malloc.h>
#endif

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QList>
#include <QTreeView>
#include <QModelIndex>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDateTime>

#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#else
#include <QtConcurrentFilter>
#include <QtConcurrentRun>
#endif

using namespace dcpp;

AsyncRunner::AsyncRunner(QObject *parent): QThread(parent){

}

AsyncRunner::~AsyncRunner()
{
    // QThread::~QThread() aborts if the thread is still running.
    if (isRunning()) {
        quit();
        wait(5000);
        if (isRunning())
            terminate();
        wait(1000);
    }
}

void AsyncRunner::run(){
    runFunc();
}

void AsyncRunner::setRunFunction(const std::function<void()> &f){
    runFunc = f;
}

ShareBrowser::ShareBrowser(UserPtr _user, const QString &_file, const QString &_jump_to):
        QWidget(MainWindow::getInstance()),
        proxy(nullptr),
        file(_file),
        jump_to(_jump_to),
        listing(HintedUser(_user, "")),
        user(_user),
        current_size(0),
        tree_model(nullptr),
        list_model(nullptr),
        tree_root(nullptr),
        list_root(nullptr)

{
    nick = WulforUtil::getInstance()->getNicks(user->getCID());

    if (nick.indexOf(_q(user->getCID().toBase32()) >= nullptr)) { // User offline
        nick = _q(ClientManager::getInstance()->getNicks(HintedUser(user, ""))[0]);

        QFileInfo info(_file);

        nick = info.baseName().left(info.baseName().indexOf("."));

        if (nick == "files")
            title = tr("Own files");
        else
            title = tr("Listing: ") + nick;
    }

    connect(this, SIGNAL(die(QString)), this, SLOT(slotDie(QString)), Qt::QueuedConnection);

    AsyncRunner *runner = new AsyncRunner(this);

    runner->setRunFunction([this]() { this->buildList(); });
    connect(runner, SIGNAL(finished()), this, SLOT(init()), Qt::QueuedConnection);
    connect(runner, SIGNAL(finished()), runner, SLOT(deleteLater()), Qt::QueuedConnection);

    runner->start();

    setState(state() | ArenaWidget::Hidden);
 }

ShareBrowser::~ShareBrowser(){
    delete tree_model;
    delete list_model;
    delete arena_menu;

    pathHistory.clear();

    Menu::deleteInstance();

#if (HAVE_MALLOC_TRIM)
    malloc_trim(0);
#endif
}

void ShareBrowser::closeEvent(QCloseEvent *e){
    save();

    QWidget::closeEvent(e);
}

bool ShareBrowser::eventFilter(QObject *obj, QEvent *e){
    QTreeView *tree_view = qobject_cast<QTreeView*>(obj);

    if (tree_view && (e->type() == QEvent::KeyRelease)){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        if (k_e->key() == Qt::Key_Enter || k_e->key() == Qt::Key_Return)
            goDown(tree_view);
        else if (k_e->key() == Qt::Key_Backspace)
            goUp(tree_view);
    }
    else if (static_cast<LineEdit*>(obj) == lineEdit_FILTER && (e->type() == QEvent::KeyRelease)){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        if (k_e->key() == Qt::Key_Escape){
            slotClearFilters();
            return true;
        }
    }

    return QWidget::eventFilter(obj, e);
}

QString ShareBrowser::getArenaTitle(){
    return title;
}

QString ShareBrowser::getArenaShortTitle(){
    return getArenaTitle();
}

QWidget *ShareBrowser::getWidget(){
    return this;
}

QMenu  *ShareBrowser::getMenu(){
    return arena_menu;
}

