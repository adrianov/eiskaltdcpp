/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "UserListModelSort.h"

#include <QtAlgorithms>
#include <QtGlobal>

namespace {

template <Qt::SortOrder order>
struct Compare {
    typedef bool (*AttrComp)(const UserListItem* l, const UserListItem* r);

    void static sort(unsigned column, QList<UserListItem*>& items) {
        if (column > COLUMN_EMAIL)
            return;

        std::stable_sort(items.begin(), items.end(), attrs[column]);
    }

    QList<UserListItem*>::iterator static insertSorted(unsigned column, QList<UserListItem*>& items, UserListItem* item) {
        if (column > COLUMN_EMAIL)
            return items.end();

        return std::lower_bound(items.begin(), items.end(), item, attrs[column] );
    }

private:
    template <typename T, T (UserListItem::*attr)() const >
    bool static AttrCmp(const UserListItem * l, const UserListItem * r) {
        if (l->isOP() != r->isOP())
            return l->isOP();
        else if (l->isFav() != r->isFav())
            return l->isFav();
        else
            return Cmp((const_cast<UserListItem*>(l)->*attr)(), (const_cast<UserListItem*>(r)->*attr)());
    }

    bool static IPCmp(const UserListItem * l, const UserListItem * r) {
        if (l->isOP() != r->isOP())
            return l->isOP();
        else if (!(l->getIP().isEmpty() || r->getIP().isEmpty())){
            QString ip1 = l->getIP();
            QString ip2 = r->getIP();

            quint32 l_ip = ip1.section('.',0,0).toULong();
            l_ip <<= 8;
            l_ip |= ip1.section('.',1,1).toULong();
            l_ip <<= 8;
            l_ip |= ip1.section('.',2,2).toULong();
            l_ip <<= 8;
            l_ip |= ip1.section('.',3,3).toULong();

            quint32 r_ip = ip2.section('.',0,0).toULong();
            r_ip <<= 8;
            r_ip |= ip2.section('.',1,1).toULong();
            r_ip <<= 8;
            r_ip |= ip2.section('.',2,2).toULong();
            r_ip <<= 8;
            r_ip |= ip2.section('.',3,3).toULong();

            return Cmp(l_ip, r_ip);
        }

        return false;
    }

    template <typename T>
    inline bool static Cmp(const T& l, const T& r) __attribute__((always_inline));

    static AttrComp attrs[8];
};

template <Qt::SortOrder order>
typename Compare<order>::AttrComp Compare<order>::attrs[8]  = {     AttrCmp<QString, &UserListItem::getNick>,
                                                                    AttrCmp<qulonglong, &UserListItem::getShare>,
                                                                    AttrCmp<qulonglong, &UserListItem::getShare>,
                                                                    AttrCmp<QString, &UserListItem::getComment>,
                                                                    AttrCmp<QString, &UserListItem::getTag>,
                                                                    AttrCmp<QString, &UserListItem::getConnection>,
                                                                    IPCmp,
                                                                    AttrCmp<QString, &UserListItem::getEmail> };

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

template <> template <>
bool inline Compare<Qt::AscendingOrder>::Cmp(const QString& l, const QString& r) {
    return Cmp(QString::localeAwareCompare(l, r), 0);
}

template <> template <>
bool inline Compare<Qt::DescendingOrder>::Cmp(const QString& l, const QString& r) {
    return Cmp(QString::localeAwareCompare(l, r), 0);
}

} // namespace

void userListSort(QList<UserListItem*> &items, int column, Qt::SortOrder order) {
    if (order == Qt::AscendingOrder)
        Compare<Qt::AscendingOrder>::sort(column, items);
    else
        Compare<Qt::DescendingOrder>::sort(column, items);
}

QList<UserListItem*>::iterator userListInsertSorted(QList<UserListItem*> &items, UserListItem *item,
                                                    int column, Qt::SortOrder order) {
    if (order == Qt::AscendingOrder)
        return Compare<Qt::AscendingOrder>::insertSorted(column, items, item);
    return Compare<Qt::DescendingOrder>::insertSorted(column, items, item);
}

void UserListModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;

    if (column < 0 || column > columnCount() - 1)
        return;

    if (rootItem->childItems.size() <= 0)
        return;

    emit layoutAboutToBeChanged();
    userListSort(rootItem->childItems, column, order);
    emit layoutChanged();
}
