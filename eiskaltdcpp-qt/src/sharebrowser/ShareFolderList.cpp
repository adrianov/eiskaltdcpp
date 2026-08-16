/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "sharebrowser/ShareFolderList.h"
#include "FileBrowserModel.h"
#include "filebrowser/FileTypeCounter.h"
#include "WulforUtil.h"

#include <QDateTime>
#include <QObject>

using namespace dcpp;

namespace {

QList<QVariant> fileRowData(DirectoryListing::File *file)
{
    return QList<QVariant>()
            << _q(file->getName())
            << WulforUtil::formatBytes(file->getSize())
            << static_cast<quint64>(file->getSize())
            << _q(file->getTTH().toBase32())
            << (file->mediaInfo.bitrate > 0
                ? QVariant(static_cast<int>(file->mediaInfo.bitrate))
                : QVariant())
            << _q(file->mediaInfo.resolution)
            << _q(file->mediaInfo.video_info)
            << _q(file->mediaInfo.audio_info)
            << static_cast<quint64>(file->getHit())
            << QDateTime::fromSecsSinceEpoch(file->getTS()).toString("yyyy-MM-dd hh:mm");
}

void appendFlatFiles(FileBrowserItem *listRoot, DirectoryListing &listing,
                     DirectoryListing::Directory *dir, quint64 &totalSize,
                     SearchFileTypes::FileTypeCounter *types, ShareMediaState &media)
{
    if (!dir)
        return;

    const QString path = _q(listing.getPath(dir));
    for (const auto &file : dir->files) {
        totalSize += file->getSize();
        if (types)
            types->addFile(_q(file->getName()));
        media.noteFile(file);
        QList<QVariant> data = fileRowData(file);
        data << path;
        FileBrowserItem *child = new FileBrowserItem(data, listRoot);
        child->file = file;
        listRoot->appendChild(child);
    }
    for (const auto &sub : dir->directories)
        appendFlatFiles(listRoot, listing, sub, totalSize, types, media);
}

void countFileTypes(DirectoryListing::Directory *dir, SearchFileTypes::FileTypeCounter &types)
{
    if (!dir)
        return;
    for (const auto &file : dir->files)
        types.addFile(_q(file->getName()));
    for (const auto &sub : dir->directories)
        countFileTypes(sub, types);
}

} // namespace

ShareFolderList::ShareFolderList(FileBrowserModel *model, FileBrowserItem *root)
    : model_(model)
    , root_(root)
{
}

void ShareFolderList::showFolder(DirectoryListing::Directory *dir)
{
    if (!dir || !model_ || !root_)
        return;

    model_->beginRebuild();
    totalSize_ = 0;
    media_.reset();

    for (const auto &sub : dir->directories) {
        const quint64 size = sub->getTotalSize(true);
        totalSize_ += size;
        QList<QVariant> data;
        data << _q(sub->getName()) << WulforUtil::formatBytes(size) << size << "";
        FileBrowserItem *child = new FileBrowserItem(data, root_);
        child->dir = sub;
        root_->appendChild(child);
    }

    for (const auto &file : dir->files) {
        totalSize_ += file->getSize();
        media_.noteFile(file);
        FileBrowserItem *child = new FileBrowserItem(fileRowData(file), root_);
        child->file = file;
        root_->appendChild(child);
    }

    model_->highlightDuplicates();
    model_->endRebuild();

    SearchFileTypes::FileTypeCounter types;
    countFileTypes(dir, types);
    typeCounts_ = types.format();
}

void ShareFolderList::showFlat(DirectoryListing &listing, DirectoryListing::Directory *dir)
{
    if (!dir || !model_ || !root_)
        return;

    SearchFileTypes::FileTypeCounter types;
    model_->beginRebuild();
    totalSize_ = 0;
    media_.reset();
    appendFlatFiles(root_, listing, dir, totalSize_, &types, media_);
    model_->highlightDuplicates();
    model_->endRebuild();
    typeCounts_ = types.format();
}

QString ShareFolderList::statusText() const
{
    QString s = QObject::tr("Total size: %1").arg(WulforUtil::formatBytes(totalSize_));
    if (!typeCounts_.isEmpty())
        s += QLatin1String("; ") + typeCounts_;
    return s;
}
