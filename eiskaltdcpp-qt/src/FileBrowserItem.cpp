/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserModel.h"

FileBrowserItem::FileBrowserItem(const QList<QVariant> &data, FileBrowserItem *parent)
    : dir(nullptr)
    , file(nullptr)
    , isDuplicate(false)
    , itemData(data)
    , parentItem(parent)
{
}

FileBrowserItem::FileBrowserItem(const FileBrowserItem &in)
    : childItems(in.childItems)
    , dir(in.dir)
    , file(in.file)
    , isDuplicate(in.isDuplicate)
    , itemData(in.itemData)
    , parentItem(in.parentItem)
{
}

void FileBrowserItem::operator=(const FileBrowserItem &in){
    itemData = in.itemData;
    dir = in.dir;
    file = in.file;
    isDuplicate = in.isDuplicate;
    childItems = in.childItems;
    parentItem = in.parentItem;
}

FileBrowserItem::~FileBrowserItem()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void FileBrowserItem::appendChild(FileBrowserItem *item) {
    childItems.append(item);
}

FileBrowserItem *FileBrowserItem::child(int row) {
    return childItems.value(row);
}

int FileBrowserItem::childCount() const {
    return childItems.count();
}

int FileBrowserItem::columnCount() const {
    return itemData.count();
}

QVariant FileBrowserItem::data(int column) const {
    return itemData.value(column);
}

FileBrowserItem *FileBrowserItem::parent() {
    return parentItem;
}

int FileBrowserItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FileBrowserItem*>(this));

    return 0;
}

void FileBrowserItem::updateColumn(int column, QVariant var){
    if (column > (itemData.size()-1))
        return;

    itemData[column] = var;
}

FileBrowserItem *FileBrowserItem::nextSibling(){
    if (!parent())
        return nullptr;

    if (row() == (parent()->childCount()-1))
        return nullptr;

    return parent()->child(row()+1);
}
