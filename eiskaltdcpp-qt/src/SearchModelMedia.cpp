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
    for (auto it = media.constBegin(); it != media.constEnd(); ++it) {
        SearchItem *item = tths.value(it.key());
        if (!item)
            continue;

        setMedia(item, it.value());
        for (SearchItem *child : item->children())
            setMedia(child, it.value());

        const QModelIndex parentIdx = createIndexForItem(item);
        if (!parentIdx.isValid())
            continue;
        const QModelIndex left = index(parentIdx.row(), COLUMN_SF_BR, parentIdx.parent());
        const QModelIndex right = index(parentIdx.row(), COLUMN_SF_MAUDIO, parentIdx.parent());
        emit dataChanged(left, right);

        for (int r = 0; r < item->childCount(); ++r) {
            const QModelIndex cLeft = index(r, COLUMN_SF_BR, parentIdx);
            const QModelIndex cRight = index(r, COLUMN_SF_MAUDIO, parentIdx);
            emit dataChanged(cLeft, cRight);
        }
    }
}
