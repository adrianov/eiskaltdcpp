/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "UserListModel.h"

#include "dcpp/stdinc.h"
#include "dcpp/Util.h"

#include "WulforUtil.h"

UserListItem::UserListItem()
    : _isOp(false)
    , _isFav(false)
    , parentItem(nullptr)
    , ptr(nullptr)
{
}

UserListItem::UserListItem(UserListItem *parent, dcpp::UserPtr _ptr, const Identity& _id, const QString& _cid, bool _fav)
    : parentItem(parent)
    , ptr(_ptr)
{
    updateIdentity(_id, _cid, _fav);
}

UserListItem::~UserListItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void UserListItem::appendChild(UserListItem *item) {
    item->parentItem = this;
    childItems.append(item);
}

UserListItem *UserListItem::child(int row) {
    return childItems.value(row);
}

int UserListItem::childCount() const {
    return childItems.count();
}

int UserListItem::columnCount() const {
    return 8;
}
UserListItem *UserListItem::parent() {
    return parentItem;
}

int UserListItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<UserListItem*>(this));

    return 0;
}

qulonglong UserListItem::getShare()  const{
    return id.getBytesShared();
}

QString UserListItem::getComment()  const{
    return _q(id.getDescription());
}

QString UserListItem::getConnection()  const{
    return _q(id.getConnection());
}

QString UserListItem::getEmail()  const{
    return _q(id.getEmail());
}

QString UserListItem::getIP()  const{
    return _q(id.getIp());
}   

QString UserListItem::getNick()  const{
    return _q(id.getNick());
}

QString UserListItem::getTag()  const{
    return _q(id.getTag());
}

QString UserListItem::getCID()  const{
    return cid;
}

dcpp::UserPtr UserListItem::getUser()  const{
    return ptr;
}

bool UserListItem::isOP()  const{
    return _isOp;
}

bool UserListItem::isFav()  const{
    return _isFav;
}

bool UserListItem::isAway() const {
    return id.isAway();
}

void UserListItem::updateIdentity(const Identity& _id, const QString& _cid, bool _fav) {
    if (ptr) {
        id = _id;
        cid = _cid;
        _isOp   = id.isOp();
        _isFav  = _fav;
    }
}
