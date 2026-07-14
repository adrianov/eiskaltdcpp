/***************************************************************************
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
        WulforUtil::eiFILETYPE_UNKNOWN,
        WulforUtil::eiFILETYPE_MP3,
        WulforUtil::eiFILETYPE_ARCHIVE,
        WulforUtil::eiFILETYPE_DOCUMENT,
        WulforUtil::eiFILETYPE_APPLICATION,
        WulforUtil::eiFILETYPE_PICTURE,
        WulforUtil::eiFILETYPE_VIDEO,
        WulforUtil::eiFOLDER_BLUE,
        WulforUtil::eiFIND,
        WulforUtil::eiFILETYPE_ARCHIVE,
        WulforUtil::eiFILETYPE_VIDEO
    };
    if (type >= 0 && type < SearchManager::TYPE_LAST)
        return icons[type];
    return WulforUtil::eiFILETYPE_UNKNOWN;
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

} // namespace SearchFileTypes
