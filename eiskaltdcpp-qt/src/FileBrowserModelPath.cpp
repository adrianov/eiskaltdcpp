/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FileBrowserModel.h"
#include "WulforUtil.h"

#include <QFile>
#include <QTextStream>

#include "dcpp/UploadManager.h"
#include "dcpp/Util.h"

using namespace dcpp;

QString FileBrowserModel::createRemotePath(FileBrowserItem *item) const{
    QString s;
    FileBrowserItem * pitem;

    if (!item) {
        return s;
    }

    pitem = item;
    s = pitem->data(COLUMN_FILEBROWSER_NAME).toString();

    while ((pitem = pitem->parent())) {
        // check for root entry
        if (pitem->parent()) {
            s = pitem->data(COLUMN_FILEBROWSER_NAME).toString() + "\\" + s;
        }
    }

    return s;
}

FileBrowserItem *FileBrowserModel::createRootForPath(const QString &path, FileBrowserItem *pathRoot){
    if (path.isEmpty() || path.isNull())
        return nullptr;

    QString _path = path;
    _path.replace("\\", "/");

    QStringList list = _path.split("/", WULFOR_SKIP_EMPTY);
    FileBrowserItem *root = pathRoot?pathRoot:rootItem;

    if (list.empty() || !root)
        return nullptr;

    for (const auto &s : list){
        if (s.isEmpty())
            continue;

        if (!root)
            return nullptr;

        if (s == ".." && root->parent()){
            root = root->parent();

            continue;
        }
        else if (s == ".")
            continue;

        bool found = false;

        if (root->dir && !root->dir->directories.empty() && !root->childCount()) //Load child items
            fetchMore(createIndexForItem(root));

        for (const auto &item : root->childItems){
            if (!item->dir)
                continue;

            QString name = (item == rootItem ? "" : item->data(COLUMN_FILEBROWSER_NAME).toString());

            if (!name.compare(s, Qt::CaseSensitive)){
                root = item;
                found = true;

                break;
            }
        }

        if (!found)
            return root;
    }

    return root;
}

QModelIndex FileBrowserModel::createIndexForItem(FileBrowserItem *item){
    if (!rootItem || !item)
        return QModelIndex();

    return createIndex(item->row(), 0, item);
}

void FileBrowserModel::saveRestrictions(){
    if (!restrictionsLoaded)
        return;

    QFile f(_q(Util::getPath(Util::PATH_USER_CONFIG) + "PerFolderLimit.conf"));

    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
        QTextStream stream(&f);

        for (auto it = restrict_map.begin(); it != restrict_map.end(); ++it) {
            stream << it.value() << " " << it.key() << '\n';
        }

        stream.flush();

        f.close();

        dcpp::UploadManager::getInstance()->reloadRestrictions();
    }
}

void FileBrowserModel::loadRestrictions(){
    QFile f(_q(Util::getPath(Util::PATH_USER_CONFIG) + "PerFolderLimit.conf"));

    if (f.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream stream(&f);
        QString line = "";

        while (!(line = stream.readLine(0)).isNull()){
            QStringList list = line.split(' ');

            if (list.size() < 2)
                continue;

            bool ok = false;
            unsigned size = 0;

            size = list.at(0).toUInt(&ok);

            if (!ok)
                continue;

            QString virt_path = line.remove(0, list.at(0).length() + 1);

            if (!virt_path.startsWith('/'))
                virt_path.prepend('/');

            if (!virt_path.isEmpty() && size > 0 && !restrict_map.contains(virt_path))
                restrict_map.insert(virt_path, size);
        }

        restrictionsLoaded = true;

        f.close();
    }
}

void FileBrowserModel::updateRestriction(QModelIndex &i, unsigned size){
    FileBrowserItem *item = static_cast<FileBrowserItem*>(i.internalPointer());
    QString path = "";//virtual path
    QStringList dirs = createRemotePath(item).split("\\");

    if (dirs.size() >= 2){
        dirs.removeFirst();

        path = "/" + dirs.join("/");
    }
    else
        path = "/";

    if (!size && restrict_map.contains(path))
        restrict_map.remove(path);
    else {
        restrict_map[path] = size;
    }
}
