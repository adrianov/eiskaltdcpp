/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SearchModel.h"

namespace {

void setMedia(SearchItem *item, const QVariantMap &m)
{
    const int br = m.value(QStringLiteral("bitrate")).toInt();
    if (br > 0 && item->data(COLUMN_SF_BR).toInt() <= 0)
        item->updateColumn(COLUMN_SF_BR, br);

    auto setText = [item, &m](unsigned col, const char *key) {
        const QString v = m.value(QString::fromLatin1(key)).toString();
        if (v.isEmpty() || !item->data(col).toString().isEmpty())
            return;
        item->updateColumn(col, v);
    };
    setText(COLUMN_SF_WH, "resolution");
    setText(COLUMN_SF_MVIDEO, "video");
    setText(COLUMN_SF_MAUDIO, "audio");
}

void noteMediaFlags(const QVariantMap &m, bool &br, bool &wh, bool &video, bool &audio)
{
    if (m.value(QStringLiteral("bitrate")).toInt() > 0)
        br = true;
    if (!m.value(QStringLiteral("resolution")).toString().isEmpty())
        wh = true;
    if (!m.value(QStringLiteral("video")).toString().isEmpty())
        video = true;
    if (!m.value(QStringLiteral("audio")).toString().isEmpty())
        audio = true;
}

bool isMediaSort(int column)
{
    return column >= static_cast<int>(COLUMN_SF_BR)
            && column <= static_cast<int>(COLUMN_SF_MAUDIO);
}

} // namespace

bool SearchModel::hasMedia(const QString &tth) const
{
    SearchItem *item = tths.value(tth);
    if (!item)
        return false;
    return item->data(COLUMN_SF_BR).toInt() > 0
            || !item->data(COLUMN_SF_WH).toString().isEmpty()
            || !item->data(COLUMN_SF_MVIDEO).toString().isEmpty()
            || !item->data(COLUMN_SF_MAUDIO).toString().isEmpty();
}

void SearchModel::applyMediaByTth(const QHash<QString, QVariantMap> &media)
{
    QList<SearchItem*> updated;
    for (auto it = media.constBegin(); it != media.constEnd(); ++it) {
        SearchItem *item = tths.value(it.key());
        if (!item)
            continue;
        noteMediaFlags(it.value(), hasBitrate_, hasResolution_, hasVideo_, hasAudio_);
        setMedia(item, it.value());
        for (SearchItem *child : item->children())
            setMedia(child, it.value());
        updated.append(item);
    }
    if (updated.isEmpty())
        return;

    if (isMediaSort(sortColumn)) {
        sort(sortColumn, sortOrder);
        return;
    }

    for (SearchItem *item : updated) {
        const QModelIndex parentIdx = createIndexForItem(item);
        if (!parentIdx.isValid())
            continue;
        emit dataChanged(index(parentIdx.row(), COLUMN_SF_BR, parentIdx.parent()),
                         index(parentIdx.row(), COLUMN_SF_MAUDIO, parentIdx.parent()));
        for (int r = 0; r < item->childCount(); ++r)
            emit dataChanged(index(r, COLUMN_SF_BR, parentIdx),
                             index(r, COLUMN_SF_MAUDIO, parentIdx));
    }
}
