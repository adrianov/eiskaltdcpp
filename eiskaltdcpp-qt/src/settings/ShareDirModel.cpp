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

#include "settings/ShareDirModel.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/ShareManager.h"

#include <QDir>
#include <QFont>
#include <QMessageBox>
#include <QStack>

using namespace dcpp;

QString ShareDirModel::filePath(const QModelIndex &index) const
{
    return QDir::toNativeSeparators(QFileSystemModel::filePath(index));
}

ShareDirModel::ShareDirModel(QObject *parent)
    : QFileSystemModel(parent)
{
    setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    setRootPath(QDir::rootPath());

    for (const auto &pair : ShareManager::getInstance()->getDirectories()) {
        QString path = pair.second.c_str();
        if (path.endsWith(QDir::separator()))
            path = path.left(path.lastIndexOf(QDir::separator()));
        emit expandMe(index(path));
        checked.insert(path);
    }
}

ShareDirModel::~ShareDirModel() = default;

Qt::ItemFlags ShareDirModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QFileSystemModel::flags(index);
    if (!index.column())
        f |= Qt::ItemIsUserCheckable;

    const QString fp = filePath(index);
    const int depth = fp.split(QDir::separator()).length();
    for (const QString &file : checked) {
        if (fp.startsWith(file) && depth != file.split(QDir::separator()).length() && fp != file) {
            f &= ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            break;
        }
    }
    return f;
}

QVariant ShareDirModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QString fp = filePath(index);
    if (role == Qt::CheckStateRole && !index.column()) {
        for (const QString &f : checked) {
            if (fp.startsWith(f) && fp.length() == f.length())
                return Qt::Checked;
        }
        return checked.contains(fp) ? Qt::Checked : Qt::Unchecked;
    }
    if (role == Qt::FontRole && !index.column() && checked.contains(fp)) {
        static QFont font;
        font.setBold(true);
        return font;
    }
    return QFileSystemModel::data(index, role);
}

bool ShareDirModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() || role != Qt::CheckStateRole)
        return QFileSystemModel::setData(index, value, role);

    if (value.toInt() == Qt::Checked) {
        emit getName(index);
        return true;
    }

    try {
        QString path = filePath(index);
        if (!path.endsWith(QDir::separator()))
            path += QDir::separator();
        ShareManager::getInstance()->removeDirectory(path.toStdString());
    } catch (const Exception &) {
    }
    checked.remove(filePath(index));
    return true;
}

void ShareDirModel::setAlias(const QModelIndex &index, const QString &alias)
{
    QString fp = filePath(index);
    if (checked.contains(fp) || !QDir(fp).exists())
        return;

    checked.insert(fp);
    try {
        if (!fp.endsWith(QDir::separator()))
            fp += QDir::separator();
        ShareManager::getInstance()->addDirectory(fp.toStdString(), alias.toStdString());
    } catch (const ShareException &e) {
        QMessageBox::critical(nullptr, tr("Error"), QString::fromStdString(e.getError()));
        return;
    }

    emit dataChanged(index, index);
    emit layoutChanged();
}

void ShareDirModel::beginExpanding()
{
    for (const QString &f : checked) {
        QStack<QModelIndex> stack;
        for (QModelIndex i = index(f); i.isValid(); i = i.parent())
            stack.push(i);
        while (!stack.isEmpty())
            emit expandMe(stack.pop());
    }
}
