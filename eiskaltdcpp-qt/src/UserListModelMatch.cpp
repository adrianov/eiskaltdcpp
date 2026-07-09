/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "UserListModel.h"

QStringList UserListModel::matchNicksContaining(const QString & part, bool stripTags) const {
    Q_UNUSED(stripTags)

    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (auto it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.contains(part)) {
                matches << (*it)->getNick();
        }
    }

    return matches;
}

QStringList UserListModel::matchNicksStartingWith(const QString & part, bool stripTags) const {
    Q_UNUSED(stripTags)

    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (auto it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.startsWith(part))
            matches << (*it)->getNick();
    }

    return matches;
}

QStringList UserListModel::matchNicksAny(const QString &part, bool stripTags) const{
    Q_UNUSED(stripTags)

    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (auto it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.contains(part))
            matches << (*it)->getNick();
    }

    return matches;
}

QStringList UserListModel::findItems(const QString &part, Qt::MatchFlags flags, int column) const
{
    if (column > COLUMN_EMAIL)
        return QStringList();

    QModelIndexList indexes = match(index(0, column, QModelIndex()),
                                    Qt::DisplayRole, part, -1, flags);
    QStringList items;
    for (int i = 0; i < indexes.size(); ++i) {
        QModelIndex index = indexes.at(i);
        if (index.isValid())
            items.append( index.data().toString() );
    }
    return items;
}
