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

#include <QString>

using namespace dcpp;

namespace {

void setMedia(FileBrowserItem *item, const QVariantMap &m)
{
    const int br = m.value(QStringLiteral("bitrate")).toInt();
    if (br > 0 && item->data(COLUMN_FILEBROWSER_BR).toInt() <= 0)
        item->updateColumn(COLUMN_FILEBROWSER_BR, br);

    auto setText = [item, &m](int col, const char *key) {
        const QString v = m.value(QString::fromLatin1(key)).toString();
        if (v.isEmpty() || !item->data(col).toString().isEmpty())
            return;
        item->updateColumn(col, v);
    };
    setText(COLUMN_FILEBROWSER_WH, "resolution");
    setText(COLUMN_FILEBROWSER_MVIDEO, "video");
    setText(COLUMN_FILEBROWSER_MAUDIO, "audio");

    if (!item->file)
        return;
    MediaInfo &mi = item->file->mediaInfo;
    if (br > 0 && mi.bitrate == 0)
        mi.bitrate = static_cast<uint16_t>(qMin(br, 0xffff));
    const QString res = m.value(QStringLiteral("resolution")).toString();
    if (!res.isEmpty() && mi.resolution.empty())
        mi.resolution = _tq(res);
    const QString video = m.value(QStringLiteral("video")).toString();
    if (!video.isEmpty() && mi.video_info.empty())
        mi.video_info = _tq(video);
    const QString audio = m.value(QStringLiteral("audio")).toString();
    if (!audio.isEmpty() && mi.audio_info.empty())
        mi.audio_info = _tq(audio);
}

bool isMediaSort(int column)
{
    return column >= COLUMN_FILEBROWSER_BR && column <= COLUMN_FILEBROWSER_MAUDIO;
}

} // namespace

QStringList FileBrowserModel::applyMediaByTth(const QHash<QString, QVariantMap> &media)
{
    QStringList applied;
    if (!rootItem || media.isEmpty())
        return applied;

    QList<FileBrowserItem*> updated;
    for (FileBrowserItem *item : rootItem->childItems) {
        if (!item || !item->file)
            continue;
        const QString tth = item->data(COLUMN_FILEBROWSER_TTH).toString();
        if (tth.isEmpty() || !media.contains(tth))
            continue;
        setMedia(item, media.value(tth));
        updated.append(item);
        if (!applied.contains(tth))
            applied << tth;
    }
    if (updated.isEmpty())
        return applied;

    if (isMediaSort(sortColumn)) {
        sort(sortColumn, sortOrder);
        return applied;
    }

    for (FileBrowserItem *item : updated) {
        const QModelIndex idx = createIndexForItem(item);
        if (!idx.isValid())
            continue;
        emit dataChanged(index(idx.row(), COLUMN_FILEBROWSER_BR),
                         index(idx.row(), COLUMN_FILEBROWSER_MAUDIO));
    }
    return applied;
}
