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

#include "downloadto/DownloadToMenu.h"
#include "downloadto/DownloadToHistory.h"
#include "WulforUtil.h"

#include <QDir>

DownloadToMenu::DownloadToMenu(const QString &title, const QString &browseLabel,
                               QWidget *parent)
    : QMenu(title, parent)
    , browseLabel(browseLabel)
{
    setIcon(WulforUtil::getInstance()->getPixmap(AppIcons::eiDOWNLOAD_AS));
}

void DownloadToMenu::refill()
{
    clear();

    const QPixmap &dirPx = WICON(AppIcons::eiFOLDER_BLUE);
    const QString aliases = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_ALIASES).toUtf8());
    const QString paths = QByteArray::fromBase64(WSGET(WS_DOWNLOADTO_PATHS).toUtf8());
    const QStringList a = aliases.split("\n", WULFOR_SKIP_EMPTY);
    const QStringList p = paths.split("\n", WULFOR_SKIP_EMPTY);
    const QStringList tempPaths = DownloadToDirHistory::get();

    if (!tempPaths.isEmpty()) {
        for (const auto &t : tempPaths) {
            QAction *act = addAction(dirPx, QDir(t).dirName());
            act->setToolTip(t);
            act->setData(t);
        }
        addSeparator();
    }

    if (a.size() == p.size() && !a.isEmpty()) {
        for (int i = 0; i < a.size(); i++) {
            QAction *act = addAction(a.at(i));
            act->setData(p.at(i));
            act->setIcon(dirPx);
        }
        addSeparator();
    }

    QAction *browse = addAction(dirPx, browseLabel);
    browse->setData(QString());
}

bool DownloadToMenu::takeTarget(QAction *chosen, QString &target) const
{
    if (!chosen || !actions().contains(chosen))
        return false;
    target = chosen->data().toString();
    return true;
}
