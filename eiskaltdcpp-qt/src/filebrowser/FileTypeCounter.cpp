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
#include "filebrowser/AdultVideo.h"
#include "SearchFileTypes.h"
#include "AppTheme.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>

using namespace dcpp;

namespace SearchFileTypes {

void fillListingCombo(QComboBox *combo, const ListingTypes &types)
{
    if (!combo)
        return;

    combo->clear();
    addTypeItem(combo, SearchManager::TYPE_ANY);

    const bool hasVideo = types.typeIds.contains(SearchManager::TYPE_VIDEO);
    for (int i = SearchManager::TYPE_AUDIO; i < SearchManager::TYPE_LAST; ++i) {
        if (i == SearchManager::TYPE_TTH || i == SearchManager::TYPE_AUDIO_VIDEO)
            continue;
        if (i == SearchManager::TYPE_DIRECTORY) {
            if (types.hasDirs)
                addTypeItem(combo, i);
            continue;
        }
        if (!types.typeIds.contains(i))
            continue;
        addTypeItem(combo, i);
        if (i == SearchManager::TYPE_AUDIO && !hasVideo)
            addTypeItem(combo, SearchManager::TYPE_AUDIO_VIDEO);
        if (i != SearchManager::TYPE_VIDEO)
            continue;
        if (types.hasAdultVideo) {
            combo->addItem(combo->itemIcon(combo->count() - 1),
                           QCoreApplication::translate("SearchFileTypes", "Adult Video"),
                           adultVideoType());
        }
        addTypeItem(combo, SearchManager::TYPE_AUDIO_VIDEO);
    }
    for (const QString &name : types.customNames)
        combo->addItem(name, SearchManager::TYPE_LAST);

    combo->setCurrentIndex(0);
    AppTheme::applyInputPalette(combo);
}

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
    if (!hasAdultVideo_ && videoExts_.contains(ext, Qt::CaseInsensitive)
            && AdultVideo::matches(fileName, path))
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
