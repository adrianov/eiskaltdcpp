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

#include "SideBar.h"
#include "WulforUtil.h"

#include <QApplication>
#include <QFont>

namespace {

QVariant boldTitleFont(ArenaWidget *awgt)
{
    if (!awgt || !awgt->titleBold())
        return QVariant();
    // Default QFont() can paint invisible on Fusion/macOS; bold the app font.
    QFont font = qApp->font();
    font.setBold(true);
    return font;
}

} // namespace

QVariant SideBarModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() == 1)
        return QVariant();

    const SideBarItem *item = static_cast<SideBarItem*>(index.internalPointer());
    ArenaWidget *awgt = item->getWidget();

    switch (role) {
    case Qt::DecorationRole:
        return WulforUtil::scalePixmap(awgt ? awgt->getPixmap() : item->pixmap, 18);
    case Qt::DisplayRole:
        return awgt ? awgt->getArenaShortTitle() : item->title;
    case Qt::TextAlignmentRole:
        return static_cast<uint>(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::FontRole:
        return boldTitleFont(awgt);
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:
    case Qt::ToolTipRole:
        return awgt
            ? QVariant(WulforUtil::getInstance()->compactToolTipText(awgt->getArenaTitle(), 60, "\n"))
            : QVariant(item->title);
    }
    return QVariant();
}

QVariant SideBarModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return (QList<QVariant>() << tr("Widgets"));
}

QModelIndex SideBarModel::index(int row, int column, const QModelIndex &parent)
const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    SideBarItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<SideBarItem*>(parent.internalPointer());

    SideBarItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex SideBarModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SideBarItem *childItem = static_cast<SideBarItem*>(index.internalPointer());
    SideBarItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SideBarModel::rowCount(const QModelIndex &parent) const
{
    SideBarItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<SideBarItem*>(parent.internalPointer());

    return parentItem->childCount();
}


void SideBarModel::sort(int column, Qt::SortOrder order) {
    Q_UNUSED(column)
    Q_UNUSED(order)
    return;
}

void SideBarModel::redraw() {
    auto maybeNotify = [this](ArenaWidget *awgt, SideBarItem *item) {
        if (!awgt || !item)
            return;
        const QString title = awgt->getArenaShortTitle();
        const quint64 iconKey = awgt->getPixmap().cacheKey();
        const bool bold = awgt->titleBold();
        const auto prev = redrawCache.constFind(awgt);
        if (prev != redrawCache.constEnd()
                && prev->title == title
                && prev->iconKey == iconKey
                && prev->bold == bold)
            return;
        redrawCache.insert(awgt, {title, iconKey, bold});
        const QModelIndex idx = createIndex(item->row(), 0, item);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    };

    for (auto it = items.constBegin(); it != items.constEnd(); ++it) {
        SideBarItem *item = it.value();
        if (!item || !item->parent())
            continue;
        maybeNotify(it.key(), item);
    }
    for (auto it = roots.constBegin(); it != roots.constEnd(); ++it) {
        SideBarItem *root = it.value();
        if (!root || !root->getWidget())
            continue;
        maybeNotify(root->getWidget(), root);
    }
}

