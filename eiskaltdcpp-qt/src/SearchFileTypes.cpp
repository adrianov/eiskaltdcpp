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

using namespace dcpp;

namespace SearchFileTypes {

namespace {

QStringList toExtList(const StringList &exts)
{
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

WulforUtil::Icons iconForType(int type)
{
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

const int kAdultVideoType = SearchManager::TYPE_LAST + 1;

} // namespace

void addTypeItem(QComboBox *combo, int typeId)
{
    combo->addItem(WICON(iconForType(typeId)), _q(SearchManager::getTypeStr(typeId)), typeId);
}

void fillCombo(QComboBox *combo, bool forSearch)
{
    if (!combo)
        return;

    combo->clear();
    for (int i = SearchManager::TYPE_ANY; i < SearchManager::TYPE_LAST; i++) {
        if (!forSearch && (i == SearchManager::TYPE_DIRECTORY || i == SearchManager::TYPE_TTH))
            continue;
        addTypeItem(combo, i);
    }
    for (const auto &entry : SettingsManager::getInstance()->getSearchTypes()) {
        if (!isNumberedType(entry.first))
            combo->addItem(_q(entry.first), SearchManager::TYPE_LAST);
    }
    combo->setCurrentIndex(0);
    AppTheme::applyInputPalette(combo);
}

int adultVideoType()
{
    return kAdultVideoType;
}

bool isAdultVideoType(int typeIndex)
{
    return typeIndex == kAdultVideoType;
}

QStringList extensionsFor(int typeIndex, const QString &typeName)
{
    if (typeIndex == kAdultVideoType)
        typeIndex = SearchManager::TYPE_VIDEO;
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
