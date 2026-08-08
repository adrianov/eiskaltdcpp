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

#include "ShareBrowser.h"
#include "FileBrowserModel.h"

#include <QHash>
#include <QVariant>
#include <QVariantMap>

void ShareBrowser::applyMediaEnrich(const QVariant &packed)
{
    if (!list_model || !folderList)
        return;

    const QVariantMap outer = packed.toMap();
    QHash<QString, QVariantMap> media;
    for (auto it = outer.constBegin(); it != outer.constEnd(); ++it)
        media.insert(it.key(), it.value().toMap());
    if (media.isEmpty())
        return;

    const QStringList applied = list_model->applyMediaByTth(media);
    if (applied.isEmpty())
        return;
    for (const QString &tth : applied)
        folderList->noteMedia(media.value(tth));
    applyOptionalColumns();
}
