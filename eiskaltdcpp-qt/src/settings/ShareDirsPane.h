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
#include <QModelIndex>
#include <QPoint>

class QLabel;
class QTreeView;
class QTreeWidget;
class QWidget;
class ShareDirModel;
class SimpleShareTree;

/** Preferences → Sharing folder lists (tree model + simple flat list). */
class ShareDirsPane : public QObject {
    Q_OBJECT
public:
    ShareDirsPane(QTreeView *treeView, QTreeWidget *simpleTree, QLabel *totalLabel,
                  QWidget *host);

    void showSimpleMenu(const QPoint &pos);
    void setSimpleMode(bool simple);
    void refreshTotals();
    void recreateShare();
    void setShareHidden(bool share);
    void askAlias(const QModelIndex &index);

private:
    void ensureModel();

    QTreeView *treeView_;
    QTreeWidget *simpleWidget_;
    QLabel *totalLabel_;
    QWidget *host_;
    ShareDirModel *model_ = nullptr;
    SimpleShareTree *simpleTree_ = nullptr;
};
