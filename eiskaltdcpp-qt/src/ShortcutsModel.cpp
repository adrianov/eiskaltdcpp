/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ShortcutsModel.h"
#include "ShortcutManager.h"
#include "MainWindow.h"

#include <QAction>
#include <QKeySequence>
#include <QMap>

ShortcutsModel::ShortcutsModel(QObject *parent) : QAbstractItemModel(parent)
{
    rootItem = new ShortcutItem(nullptr);

    MainWindow *MW = MainWindow::getInstance();
    const QMap<QString, QKeySequence> shs = ShortcutManager::getInstance()->getShortcuts();

    for (auto it = shs.begin(); it != shs.end(); ++it) {
        const QAction *act = MW->findChild<QAction*>(it.key());

        if (!act)
            continue;

        ShortcutItem *item = new ShortcutItem(rootItem);
        item->title = act->text();
        item->shortcut = it.value().toString();

        rootItem->appendChild(item);
        items.insert(item, it.key());
    }

    emit layoutChanged();
}

ShortcutsModel::~ShortcutsModel()
{
    delete rootItem;
}

void ShortcutsModel::save()
{
    MainWindow *MW = MainWindow::getInstance();

    for (auto it = items.begin(); it != items.end(); ++it) {
        QAction *act = MW->findChild<QAction*>(it.value());

        if (!act)
            continue;

        ShortcutManager::getInstance()->updateShortcut(act, it.key()->shortcut);
    }

    ShortcutManager::getInstance()->save();
}

int ShortcutsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootItem->childCount();

    return 0;
}

int ShortcutsModel::columnCount(const QModelIndex &) const
{
    return 2;
}

Qt::ItemFlags ShortcutsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ShortcutsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ShortcutItem *item = static_cast<ShortcutItem*>(index.internalPointer());

    if (!item)
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return item->title;
        case 1: return item->shortcut;
        }
    }

    return QVariant();
}

QVariant ShortcutsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Action");
        case 1: return tr("Hotkey");
        }
    }

    return QVariant();
}

void ShortcutsModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)
    Q_UNUSED(order)
    emit layoutChanged();
}

QModelIndex ShortcutsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row > (rootItem->childCount() - 1))
        return QModelIndex();

    return createIndex(row, column, rootItem->child(row));
}

QModelIndex ShortcutsModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

ShortcutItem::ShortcutItem(ShortcutItem *parent) : parentItem(parent)
{
}

ShortcutItem::~ShortcutItem()
{
    qDeleteAll(childItems);
}

void ShortcutItem::appendChild(ShortcutItem *item)
{
    item->parentItem = this;
    childItems.append(item);
}

ShortcutItem *ShortcutItem::child(int row)
{
    return childItems.value(row);
}

int ShortcutItem::childCount() const
{
    return childItems.count();
}

int ShortcutItem::columnCount() const
{
    return 2;
}

ShortcutItem *ShortcutItem::parent()
{
    return parentItem;
}

int ShortcutItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ShortcutItem*>(this));

    return 0;
}
