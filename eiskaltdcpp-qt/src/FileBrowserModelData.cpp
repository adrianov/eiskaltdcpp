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
#include "AppTheme.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

#include "dcpp/ShareManager.h"

using namespace dcpp;

QVariant FileBrowserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FileBrowserItem *item = static_cast<FileBrowserItem*>(index.internalPointer());
    QString path;
    QStringList dirs = createRemotePath(item).split("\\");

    if (dirs.size() >= 2){
        dirs.removeFirst();
        path = "/" + dirs.join("/");
    }

    switch(role) {
        case Qt::DecorationRole:
        {
            if (item->dir && index.column() == COLUMN_FILEBROWSER_NAME)
                return WICON_SIZE(WulforUtil::eiFOLDER_BLUE, 16);
            else if (index.column() == COLUMN_FILEBROWSER_NAME)
                return WulforUtil::scalePixmap(WulforUtil::getInstance()->getPixmapForFile(item->data(COLUMN_FILEBROWSER_NAME).toString()), 16);
            break;
        }
        case Qt::DisplayRole:
        {
            if (restrict_map.contains(path) && index.column() == COLUMN_FILEBROWSER_NAME)
                return tr("%1 [%2 Gb]").arg(item->data(index.column()).toString()).arg(restrict_map[path]);

            return item->data(index.column());
        }
        case Qt::TextAlignmentRole:
        {
            bool align_right = (index.column() == COLUMN_FILEBROWSER_ESIZE) || (index.column() == COLUMN_FILEBROWSER_SIZE);
            return align_right ? Qt::AlignRight : Qt::AlignLeft;
        }
        case Qt::BackgroundRole:
        {
            if (item->isDuplicate)
                return qApp->palette().color(QPalette::Highlight);

            if (item->dir || ownList)
                break;

            TTHValue t(_tq(item->data(COLUMN_FILEBROWSER_TTH).toString()));
            if (ShareManager::getInstance()->isTTHShared(t))
                return AppTheme::sharedFileHighlight();
            break;
        }
        case Qt::ToolTipRole:
        {
            if (item->isDuplicate && item->file){
                const QString &tth = item->data(COLUMN_FILEBROWSER_TTH).toString();
                auto it = hash.find(tth);

                if (it == hash.end())
                    break;

                dcpp::DirectoryListing::File *file = const_cast<dcpp::DirectoryListing::File*>(it.value());
                dcpp::DirectoryListing::Directory *parentDir = file->getParent();

                if (!parentDir)
                    break;

                QString dupPath;
                do {
                    dupPath = _q(parentDir->getName()) + "/" + dupPath;
                    parentDir = parentDir->getParent();
                } while (parentDir->getParent());

                return tr("File marked as a duplicate of another file: %1").arg(dupPath+_q(file->getName()));
            }

            QString tooltip;

            if (item->dir)
                tooltip = item->data(COLUMN_FILEBROWSER_NAME).toString();

            if (item->file){
                DirectoryListing::File *f = item->file;

                if (!f->mediaInfo.video_info.empty() || !f->mediaInfo.audio_info.empty()){
                    MediaInfo &mi = f->mediaInfo;

                    tooltip = tr("<b>Media Info:</b><br/>");
                    if (!f->mediaInfo.video_info.empty())
                        tooltip += tr("&nbsp;&nbsp;<b>Video:</b> %1<br/>").arg(_q(mi.video_info));
                    if (!f->mediaInfo.audio_info.empty())
                        tooltip += tr("&nbsp;&nbsp;<b>Audio:</b> %1<br/>").arg(_q(mi.audio_info));
                    if (f->mediaInfo.bitrate > 0)
                        tooltip += tr("&nbsp;&nbsp;<b>Bitrate:</b> %1<br/>").arg(mi.bitrate);
                    if (!f->mediaInfo.resolution.empty())
                        tooltip += tr("&nbsp;&nbsp;<b>Resolution:</b> %1<br/><br/>").arg(_q(mi.resolution));
                }
            }

            TTHValue t(_tq(item->data(COLUMN_FILEBROWSER_TTH).toString()));
            ShareManager *SM = ShareManager::getInstance();

            if (!ownList){
                try{
                    QString toolTip = _q(SM->toReal(SM->toVirtual(t)));
                    return tooltip + tr("File already exists: %1").arg(toolTip);
                }catch( ... ){}
            }

            if (!tooltip.isEmpty())
                return tooltip;

            break;
        }
        case Qt::FontRole:
        {
            if (restrict_map.contains(path) && index.column() == COLUMN_FILEBROWSER_NAME){
                QFont f;
                f.setBold(true);
                return f;
            }
            break;
        }
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
             << tr("Downloaded") << tr("Shared");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootData.at(section);

    return QVariant();
}
