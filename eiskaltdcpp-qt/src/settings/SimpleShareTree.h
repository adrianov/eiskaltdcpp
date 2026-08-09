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

#include <QObject>
#include <QPoint>

class QTreeWidget;
class QWidget;

/** Flat share-folder list (simple mode): refresh rows and context menu. */
class SimpleShareTree : public QObject {
    Q_OBJECT
public:
    SimpleShareTree(QTreeWidget *tree, QWidget *host);

    void refresh();
    void showMenu(const QPoint &pos);

private:
    void addDirectory();
    void removeSelected();
    void renameSelected();

    QTreeWidget *tree_;
    QWidget *host_;
};
