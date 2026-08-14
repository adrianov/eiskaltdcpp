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

#pragma once

#include <QAction>
#include <QMenu>
#include <QString>

/** "Download to..." submenu: recent dirs, saved aliases, and Browse. */
class DownloadToMenu : public QMenu {
public:
    DownloadToMenu(const QString &title, const QString &browseLabel,
                   QWidget *parent = nullptr);

    void refill();
    bool takeTarget(QAction *chosen, QString &target) const;

private:
    QString browseLabel;
};
