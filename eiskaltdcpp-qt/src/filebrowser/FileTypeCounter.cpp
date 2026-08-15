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

#include "filebrowser/FileTypeCounter.h"
#include "SearchFileTypes.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"

#include <QFileInfo>

using namespace dcpp;

namespace SearchFileTypes {

FileTypeCounter::FileTypeCounter()
{
    videoExts_ = extensionsFor(SearchManager::TYPE_VIDEO);
    auto add = [this](int typeId, const QString &name) {
        const QStringList exts = extensionsFor(typeId, name);
        if (!exts.isEmpty())
            buckets.push_back({typeId, name, exts, 0});
    };

    for (int i = SearchManager::TYPE_AUDIO; i < SearchManager::TYPE_DIRECTORY; ++i)
        add(i, _q(SearchManager::getTypeStr(i)));
    add(SearchManager::TYPE_CD_IMAGE, _q(SearchManager::getTypeStr(SearchManager::TYPE_CD_IMAGE)));

    for (const auto &entry : SettingsManager::getInstance()->getSearchTypes()) {
        if (!isNumberedType(entry.first))
            add(SearchManager::TYPE_LAST, _q(entry.first));
    }
}

void FileTypeCounter::addFile(const QString &fileName, const QString &path)
{
    const QString ext = QFileInfo(fileName).suffix().toUpper();
    if (ext.isEmpty())
        return;
    if (!hasAdultVideo_ && matchesFile(fileName, path, videoExts_, true))
        hasAdultVideo_ = true;
    for (Bucket &b : buckets) {
        if (b.exts.contains(ext, Qt::CaseInsensitive)) {
            ++b.count;
            return;
        }
    }
}

void FileTypeCounter::fillListing(ListingTypes &out) const
{
    out.hasAdultVideo = hasAdultVideo_;
    out.typeIds.clear();
    out.customNames.clear();
    for (const Bucket &b : buckets) {
        if (b.count <= 0)
            continue;
        if (b.typeId >= SearchManager::TYPE_LAST)
            out.customNames << b.name;
        else
            out.typeIds << b.typeId;
    }
}

QString FileTypeCounter::format() const
{
    QStringList parts;
    for (const Bucket &b : buckets) {
        if (b.count > 0)
            parts << QString("%1: %2").arg(b.name).arg(b.count);
    }
    return parts.join(QLatin1String("; "));
}

} // namespace SearchFileTypes
