/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SettingsShortcuts.h"
#include "MainWindow.h"
#include "ShortcutGetter.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include <QByteArray>
#include <QHeaderView>

static const QString &TREEVIEW_STATE_KEY = "settings-shortcuts-tableview-state";

SettingsShortcuts::SettingsShortcuts(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    model = new ShortcutsModel(this);
    treeView->setModel(model);
    WulforUtil::restoreTreeHeader(treeView->header(), QByteArray::fromBase64(WSGET(TREEVIEW_STATE_KEY).toUtf8()));

    connect(treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(slotIndexClicked(QModelIndex)));
}

SettingsShortcuts::~SettingsShortcuts()
{
    model->deleteLater();
}

void SettingsShortcuts::ok()
{
    model->save();

    WSSET(TREEVIEW_STATE_KEY, treeView->header()->saveState().toBase64());
}

void SettingsShortcuts::slotIndexClicked(const QModelIndex &index)
{
    if (!(index.isValid() && index.column() == 1))
        return;

    ShortcutItem *item = reinterpret_cast<ShortcutItem*>(index.internalPointer());

    if (!item)
        return;

    ShortcutGetter getter(MainWindow::getInstance());
    const QString ret = getter.exec(item->shortcut);

    if (ret.isNull())
        return;

    item->shortcut = ret;
    model->repaint();
}
