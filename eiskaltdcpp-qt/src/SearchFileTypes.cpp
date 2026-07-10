/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "SearchFileTypes.h"
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

void fillCombo(QComboBox *combo) {
    if (!combo)
        return;

    combo->clear();

    QStringList filetypes;
    for (int i = SearchManager::TYPE_ANY; i < SearchManager::TYPE_LAST; i++)
        filetypes << _q(SearchManager::getTypeStr(i));

    const SettingsManager::SearchTypes &searchTypes = SettingsManager::getInstance()->getSearchTypes();
    for (const auto &i : searchTypes) {
        const string &type = i.first;
        if (!(type.size() == 1 && type[0] >= '1' && type[0] <= '7'))
            filetypes << _q(type);
    }

    combo->addItems(filetypes);
    combo->setCurrentIndex(0);

    QList<WulforUtil::Icons> icons;
    icons << WulforUtil::eiFILETYPE_UNKNOWN  << WulforUtil::eiFILETYPE_MP3
          << WulforUtil::eiFILETYPE_ARCHIVE  << WulforUtil::eiFILETYPE_DOCUMENT
          << WulforUtil::eiFILETYPE_APPLICATION << WulforUtil::eiFILETYPE_PICTURE
          << WulforUtil::eiFILETYPE_VIDEO    << WulforUtil::eiFOLDER_BLUE
          << WulforUtil::eiFIND              << WulforUtil::eiFILETYPE_ARCHIVE;

    for (int i = 0; i < icons.size() && i < combo->count(); i++)
        combo->setItemIcon(i, WICON(icons.at(i)));
}

QStringList extensionsFor(int typeIndex, const QString &typeName) {
    try {
        if ((typeIndex > SearchManager::TYPE_ANY && typeIndex < SearchManager::TYPE_DIRECTORY) ||
            typeIndex == SearchManager::TYPE_CD_IMAGE) {
            // Defaults use keys '1'..'6' for Audio..Video and '7' for CD Image.
            const char key = (typeIndex == SearchManager::TYPE_CD_IMAGE)
                    ? '7' : static_cast<char>('0' + typeIndex);
            return toExtList(SettingsManager::getInstance()->getExtensions(string(1, key)));
        }
        if (typeIndex >= SearchManager::TYPE_LAST && !typeName.isEmpty())
            return toExtList(SettingsManager::getInstance()->getExtensions(_tq(typeName)));
    }
    catch (const SearchTypeException&) {
    }
    return QStringList();
}

} // namespace SearchFileTypes
