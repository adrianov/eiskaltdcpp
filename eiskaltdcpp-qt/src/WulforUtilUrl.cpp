/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"
#include "WulforSettings.h"
#include "MainWindow.h"
#include "Magnet.h"
#include "SearchFrame.h"
#include "ArenaWidgetFactory.h"

#include "dcpp/SettingsManager.h"
#include "extra/magnet.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

using namespace dcpp;

bool WulforUtil::openUrl(const QString &url){
    if (url.startsWith("http://") || url.startsWith("www.") || url.startsWith(("ftp://")) || url.startsWith("https://")){
        if (!SETTING(MIME_HANDLER).empty())
            QProcess::startDetached(_q(SETTING(MIME_HANDLER)), QStringList(url));
        else
            QDesktopServices::openUrl(QUrl::fromEncoded(url.toUtf8()));
    }
    else if (url.startsWith("adc://") || url.startsWith("adcs://")){
        MainWindow::getInstance()->newHubFrame(url, "UTF-8");
    }
    else if (url.startsWith("dchub://") || url.startsWith("nmdcs://")){
        MainWindow::getInstance()->newHubFrame(url, WSGET(WS_DEFAULT_LOCALE));
    }
    else if (url.startsWith("magnet:") && url.contains("urn:tree:tiger")){
        QString magnet = url;
        Magnet *m = new Magnet(MainWindow::getInstance());

        m->setLink(magnet);
        m->exec();

        m->deleteLater();
    }
    else if (url.startsWith("magnet:")){
        const QString magnet = url;

#if QT_VERSION >= 0x050000
        QUrlQuery u;
#else
        QUrl u;
#endif

        if (!magnet.contains("+")) {
#if QT_VERSION >= 0x050000
                u.setQuery(magnet.toUtf8());
#else
                u.setEncodedUrl(magnet.toUtf8());
#endif
        } else {
            QString _l = magnet;

            _l.replace("+", "%20");
#if QT_VERSION >= 0x050000
                u.setQuery(_l.toUtf8());
#else
                u.setEncodedUrl(_l.toUtf8());
#endif
        }

        StringMap params;
        magnet::parseUri(magnet.toStdString(), params);

        if (!params["kt"].empty()) {
            const QString keywords = _q(params["kt"]);
            QString hub = _q(params["xs"]);

            if (!hub.isEmpty() && !hub.contains("://"))
                hub.prepend("dchub://");

            if (keywords.isEmpty())
                return false;

            if (!hub.isEmpty())
                WulforUtil::openUrl(hub);

            SearchFrame *sfr = ArenaWidgetFactory().create<SearchFrame>();
            sfr->fastSearch(keywords, false);
        }
        else {
            if (!SETTING(MIME_HANDLER).empty())
                QProcess::startDetached(_q(SETTING(MIME_HANDLER)), QStringList(url));
            else
                QDesktopServices::openUrl(QUrl::fromEncoded(url.toUtf8()));
        }
    }
    else
        return false;

    return true;
}

bool WulforUtil::revealPath(const QString &path)
{
    if (path.isEmpty())
        return false;

    const QString localPath = QFileInfo(path).absoluteFilePath();
    if (localPath.isEmpty())
        return false;

#if defined(Q_OS_MAC)
    return QProcess::startDetached("open", QStringList{"-R", localPath});
#elif defined(Q_OS_WIN)
    return QProcess::startDetached("explorer", QStringList{"/select,", QDir::toNativeSeparators(localPath)});
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(localPath).absolutePath()));
#endif
}

