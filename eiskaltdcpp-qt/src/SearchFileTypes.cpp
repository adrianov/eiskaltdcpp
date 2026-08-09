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

#include "SearchFileTypes.h"
#include "AppTheme.h"
#include "WulforUtil.h"

#include "dcpp/stdinc.h"
#include "dcpp/SearchManager.h"
#include "dcpp/SettingsManager.h"

#include <QComboBox>
#include <QFileInfo>

using namespace dcpp;

namespace SearchFileTypes {

static QStringList toExtList(const StringList &exts) {
    QStringList out;
    for (const auto &e : exts) {
        QString s = _q(e).trimmed().toUpper();
        if (s.startsWith(QLatin1Char('.')))
            s = s.mid(1);
        if (!s.isEmpty())
            out << s;
    }
    return out;
}

static WulforUtil::Icons iconForType(int type) {
    static const WulforUtil::Icons icons[SearchManager::TYPE_LAST] = {
        AppIcons::eiFILETYPE_UNKNOWN,
        AppIcons::eiFILETYPE_MP3,
        AppIcons::eiFILETYPE_ARCHIVE,
        AppIcons::eiFILETYPE_DOCUMENT,
        AppIcons::eiFILETYPE_APPLICATION,
        AppIcons::eiFILETYPE_PICTURE,
        AppIcons::eiFILETYPE_VIDEO,
        AppIcons::eiFOLDER_BLUE,
        AppIcons::eiFIND,
        AppIcons::eiFILETYPE_ARCHIVE,
        AppIcons::eiFILETYPE_VIDEO
    };
    if (type >= 0 && type < SearchManager::TYPE_LAST)
        return icons[type];
    return AppIcons::eiFILETYPE_UNKNOWN;
}

void fillCombo(QComboBox *combo, bool forSearch) {
    if (!combo)
        return;

    combo->clear();

    for (int i = SearchManager::TYPE_ANY; i < SearchManager::TYPE_LAST; i++) {
        if (!forSearch && (i == SearchManager::TYPE_DIRECTORY || i == SearchManager::TYPE_TTH))
            continue;
        combo->addItem(WICON(iconForType(i)), _q(SearchManager::getTypeStr(i)), i);
    }

    const SettingsManager::SearchTypes &searchTypes = SettingsManager::getInstance()->getSearchTypes();
    for (const auto &entry : searchTypes) {
        const string &type = entry.first;
        if (!(type.size() == 1 && type[0] >= '1' && type[0] <= '7'))
            combo->addItem(_q(type), SearchManager::TYPE_LAST);
    }

    combo->setCurrentIndex(0);
    // Icon combos often ignore stylesheet color; re-apply after items change.
    AppTheme::applyInputPalette(combo);
}

QStringList extensionsFor(int typeIndex, const QString &typeName) {
    try {
        if ((typeIndex > SearchManager::TYPE_ANY && typeIndex < SearchManager::TYPE_DIRECTORY) ||
            typeIndex == SearchManager::TYPE_CD_IMAGE ||
            typeIndex == SearchManager::TYPE_AUDIO_VIDEO) {
            return toExtList(SearchManager::getTypeExtensions(typeIndex));
        }
        if (typeIndex >= SearchManager::TYPE_LAST && !typeName.isEmpty())
            return toExtList(SettingsManager::getInstance()->getExtensions(_tq(typeName)));
    }
    catch (const SearchTypeException&) {
    }
    return QStringList();
}

FileTypeCounter::FileTypeCounter() {
    auto add = [this](int typeId, const QString &name) {
        const QStringList exts = extensionsFor(typeId, name);
        if (!exts.isEmpty())
            buckets.push_back({name, exts, 0});
    };

    for (int i = SearchManager::TYPE_AUDIO; i < SearchManager::TYPE_DIRECTORY; ++i)
        add(i, _q(SearchManager::getTypeStr(i)));
    add(SearchManager::TYPE_CD_IMAGE, _q(SearchManager::getTypeStr(SearchManager::TYPE_CD_IMAGE)));

    for (const auto &entry : SettingsManager::getInstance()->getSearchTypes()) {
        const string &type = entry.first;
        if (type.size() == 1 && type[0] >= '1' && type[0] <= '7')
            continue;
        add(SearchManager::TYPE_LAST, _q(type));
    }
}

void FileTypeCounter::addFile(const QString &fileName) {
    const QString ext = QFileInfo(fileName).suffix().toUpper();
    if (ext.isEmpty())
        return;
    for (Bucket &b : buckets) {
        if (b.exts.contains(ext, Qt::CaseInsensitive)) {
            ++b.count;
            return;
        }
    }
}

QString FileTypeCounter::format() const {
    QStringList parts;
    for (const Bucket &b : buckets) {
        if (b.count > 0)
            parts << QString("%1: %2").arg(b.name).arg(b.count);
    }
    return parts.join(QLatin1String("; "));
}

} // namespace SearchFileTypes
