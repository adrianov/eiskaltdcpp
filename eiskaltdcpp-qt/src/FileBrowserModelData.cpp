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

#include "FileBrowserModel.h"
#include "WulforUtil.h"
#include "AppTheme.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

#include "dcpp/ShareManager.h"

using namespace dcpp;

namespace {

QString virtualPath(const FileBrowserModel *model, FileBrowserItem *item)
{
    QStringList dirs = model->createRemotePath(item).split("\\");
    if (dirs.size() < 2)
        return QString();
    dirs.removeFirst();
    return "/" + dirs.join("/");
}

QVariant nameIcon(FileBrowserItem *item, int column)
{
    if (column != COLUMN_FILEBROWSER_NAME)
        return QVariant();
    WulforUtil *wu = WulforUtil::getInstance();
    if (item->dir)
        return WulforUtil::scalePixmap(wu->getPixmapForFolder(), 16);
    return WulforUtil::scalePixmap(
            wu->getPixmapForFile(item->data(COLUMN_FILEBROWSER_NAME).toString()), 16);
}

QVariant displayCell(FileBrowserItem *item, int column, const QString &path,
                     const QMap<QString, unsigned> &restrict_map)
{
    if (restrict_map.contains(path) && column == COLUMN_FILEBROWSER_NAME)
        return FileBrowserModel::tr("%1 [%2 Gb]")
                .arg(item->data(column).toString())
                .arg(restrict_map[path]);

    // Non-media files keep bitrate 0 internally; show blank, not "0".
    if (column == COLUMN_FILEBROWSER_BR
            && item->data(COLUMN_FILEBROWSER_BR).toInt() <= 0)
        return QVariant();

    return item->data(column);
}

QVariant backgroundCell(FileBrowserItem *item, bool ownList)
{
    if (item->isDuplicate)
        return qApp->palette().color(QPalette::Highlight);
    if (item->dir || ownList)
        return QVariant();

    TTHValue t(_tq(item->data(COLUMN_FILEBROWSER_TTH).toString()));
    if (ShareManager::getInstance()->isTTHShared(t))
        return AppTheme::sharedFileHighlight();
    return QVariant();
}

QString duplicateTip(FileBrowserItem *item,
                     const QHash<QString, DirectoryListing::File*> &hash)
{
    const QString &tth = item->data(COLUMN_FILEBROWSER_TTH).toString();
    auto it = hash.find(tth);
    if (it == hash.end())
        return QString();

    DirectoryListing::File *file = const_cast<DirectoryListing::File*>(it.value());
    DirectoryListing::Directory *parentDir = file->getParent();
    if (!parentDir)
        return QString();

    QString dupPath;
    do {
        dupPath = _q(parentDir->getName()) + "/" + dupPath;
        parentDir = parentDir->getParent();
    } while (parentDir->getParent());

    return FileBrowserModel::tr("File marked as a duplicate of another file: %1")
            .arg(dupPath + _q(file->getName()));
}

QString mediaTip(DirectoryListing::File *f)
{
    if (f->mediaInfo.empty())
        return QString();

    const MediaInfo &mi = f->mediaInfo;
    QString tip = FileBrowserModel::tr("<b>Media Info:</b><br/>");
    if (!mi.video_info.empty())
        tip += FileBrowserModel::tr("&nbsp;&nbsp;<b>Video:</b> %1<br/>").arg(_q(mi.video_info));
    if (!mi.audio_info.empty())
        tip += FileBrowserModel::tr("&nbsp;&nbsp;<b>Audio:</b> %1<br/>").arg(_q(mi.audio_info));
    if (mi.bitrate > 0)
        tip += FileBrowserModel::tr("&nbsp;&nbsp;<b>Bitrate:</b> %1<br/>").arg(mi.bitrate);
    if (!mi.resolution.empty())
        tip += FileBrowserModel::tr("&nbsp;&nbsp;<b>Resolution:</b> %1<br/><br/>")
                .arg(_q(mi.resolution));
    return tip;
}

QString sharedPathTip(FileBrowserItem *item)
{
    TTHValue t(_tq(item->data(COLUMN_FILEBROWSER_TTH).toString()));
    ShareManager *sm = ShareManager::getInstance();
    try {
        return FileBrowserModel::tr("File already exists: %1")
                .arg(_q(sm->toReal(sm->toVirtual(t))));
    } catch (...) {
        return QString();
    }
}

QVariant toolTipCell(FileBrowserItem *item, bool ownList,
                     const QHash<QString, DirectoryListing::File*> &hash)
{
    if (item->isDuplicate && item->file) {
        const QString tip = duplicateTip(item, hash);
        if (!tip.isEmpty())
            return tip;
    }

    QString tip;
    if (item->dir)
        tip = item->data(COLUMN_FILEBROWSER_NAME).toString();
    if (item->file) {
        const QString media = mediaTip(item->file);
        if (!media.isEmpty())
            tip = media;
    }

    if (!ownList) {
        const QString existing = sharedPathTip(item);
        if (!existing.isEmpty())
            return tip + existing;
    }

    if (!tip.isEmpty())
        return tip;
    return QVariant();
}

QVariant restrictedBold(const QString &path, int column,
                        const QMap<QString, unsigned> &restrict_map)
{
    if (!restrict_map.contains(path) || column != COLUMN_FILEBROWSER_NAME)
        return QVariant();
    QFont f;
    f.setBold(true);
    return f;
}

} // namespace

QVariant FileBrowserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());
    const int col = index.column();
    const QString path = virtualPath(this, item);

    switch (role) {
        case Qt::DecorationRole:
            return nameIcon(item, col);
        case Qt::DisplayRole:
            return displayCell(item, col, path, restrict_map);
        case Qt::TextAlignmentRole:
            return (col == COLUMN_FILEBROWSER_ESIZE || col == COLUMN_FILEBROWSER_SIZE)
                    ? Qt::AlignRight : Qt::AlignLeft;
        case Qt::BackgroundRole:
            return backgroundCell(item, ownList);
        case Qt::ToolTipRole:
            return toolTipCell(item, ownList, hash);
        case Qt::FontRole:
            return restrictedBold(path, col, restrict_map);
        default:
            break;
    }

    return QVariant();
}

QVariant FileBrowserModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Size") << tr("Exact size") << QString("TTH")
             << tr("Bitrate") << tr("Resolution") << tr("Video") << tr("Audio")
             << tr("Downloaded") << tr("Shared") << tr("Path");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole
            && section >= 0 && section < rootData.size())
        return rootData.at(section);

    return QVariant();
}
