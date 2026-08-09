/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "mainwindow/WindowShare.h"

#include "MainWindow.h"
#include "MainWindowPrivate.h"
#include "ArenaWidgetFactory.h"
#include "FileHasher.h"
#include "HashProgress.h"
#include "Magnet.h"
#include "ShareBrowser.h"
#include "WulforUtil.h"

#include "dcpp/ClientManager.h"
#include "dcpp/DirectoryListing.h"
#include "dcpp/QueueManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/Util.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>

using namespace dcpp;

void WindowShare::browseOwnFiles()
{
    browseOwnFilelist();
}

void WindowShare::browseFilelist()
{
    QString file = QFileDialog::getOpenFileName(host, MainWindow::tr("Choose file to open"),
                QString::fromStdString(Util::getPath(Util::PATH_FILE_LISTS)),
                MainWindow::tr("Modern XML Filelists") + " (*.xml.bz2);;" +
                MainWindow::tr("Modern XML Filelists uncompressed") + " (*.xml);;" +
                MainWindow::tr("All files") + " (*)");

    if (file.isEmpty())
        return;

    file = QDir::toNativeSeparators(file);
    UserPtr user = DirectoryListing::getUserFromFilename(_tq(file));

    if (user)
        ArenaWidgetFactory().create<ShareBrowser, UserPtr, QString, QString>(user, file, "");
    else
        host->setStatusMessage(MainWindow::tr("Unable to load file list: Invalid file list name"));
}

void WindowShare::browseOwnFilelist()
{
    UserPtr user = ClientManager::getInstance()->getMe();
    QString file = QString::fromStdString(ShareManager::getInstance()->getOwnListFile());
    ArenaWidgetFactory().create<ShareBrowser, UserPtr, QString, QString>(user, file, "");
}

void WindowShare::matchAllLists()
{
    QueueManager::getInstance()->matchAllListings();
}

void WindowShare::showShareBrowser(UserPtr usr, const QString &file, const QString &jump_to)
{
    ArenaWidgetFactory().create<ShareBrowser, UserPtr, QString, QString>(usr, file, jump_to);
}

void WindowShare::openLogFile()
{
    QString f = QFileDialog::getOpenFileName(host, MainWindow::tr("Open log file"),
                                             _q(SETTING(LOG_DIRECTORY)),
                                             MainWindow::tr("Log files (*.log);;All files (*.*)"));
    if (f.isEmpty())
        return;

    f = QDir::toNativeSeparators(f);
    if (f.startsWith("/"))
        f = "file://" + f;
    else
        f = "file:///" + f;

    QDesktopServices::openUrl(QUrl(f));
}

void WindowShare::openDownloadDirectory()
{
    QString directory = QString::fromStdString(SETTING(DOWNLOAD_DIRECTORY));
    directory.prepend(directory.startsWith("/") ? ("file://") : ("file:///"));
    QDesktopServices::openUrl(QUrl::fromEncoded(directory.toUtf8()));
}

void WindowShare::showHashProgress()
{
    host->progress_dialog()->slotAutoClose(false);
    host->progress_dialog()->show();
}

void WindowShare::runFileHasher()
{
    FileHasher *m = new FileHasher(MainWindow::getInstance());
    m->setModal(true);
    m->exec();
    delete m;
}

void WindowShare::refreshShareOrShowHash()
{
    switch (HashProgress::getHashStatus()) {
    case HashProgress::IDLE: {
        ShareManager *SM = ShareManager::getInstance();
        SM->setDirty();
        SM->refresh(true);
        host->updateHashProgressStatus();
        host->progress_dialog()->resetProgress();
        break;
    }
    case HashProgress::LISTUPDATE:
    case HashProgress::PAUSED:
    case HashProgress::DELAYED:
    case HashProgress::RUNNING:
        showHashProgress();
        break;
    default:
        break;
    }
}

void WindowShare::openMagnet()
{
    QString text = qApp->clipboard()->text(QClipboard::Clipboard);
    bool ok = false;
    text = (text.startsWith("magnet:?") ? text : "");

    QString result = QInputDialog::getText(host, MainWindow::tr("Open magnet link"),
                                           MainWindow::tr("Enter magnet link:"),
                                           QLineEdit::Normal, text, &ok);
    if (!ok)
        return;

    if (result.startsWith("magnet:?")) {
        Magnet m(host);
        m.setLink(result);
        m.exec();
    }
}

void MainWindow::browseOwnFiles()
{
    WindowShare(this, d_func()).browseOwnFiles();
}

void MainWindow::slotFileBrowseFilelist()
{
    WindowShare(this, d_func()).browseFilelist();
}

void MainWindow::slotFileMatchAllList()
{
    WindowShare(this, d_func()).matchAllLists();
}

void MainWindow::showShareBrowser(dcpp::UserPtr usr, const QString &file, const QString &jump_to)
{
    WindowShare(this, d_func()).showShareBrowser(usr, file, jump_to);
}

void MainWindow::slotFileOpenLogFile()
{
    WindowShare(this, d_func()).openLogFile();
}

void MainWindow::slotFileOpenDownloadDirectory()
{
    WindowShare(this, d_func()).openDownloadDirectory();
}

void MainWindow::slotFileBrowseOwnFilelist()
{
    WindowShare(this, d_func()).browseOwnFilelist();
}

void MainWindow::slotFileHashProgress()
{
    WindowShare(this, d_func()).showHashProgress();
}

void MainWindow::slotFileHasher()
{
    WindowShare(this, d_func()).runFileHasher();
}

void MainWindow::slotFileRefreshShareHashProgress()
{
    WindowShare(this, d_func()).refreshShareOrShowHash();
}

void MainWindow::slotOpenMagnet()
{
    WindowShare(this, d_func()).openMagnet();
}
