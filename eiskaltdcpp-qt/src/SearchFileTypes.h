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

#pragma once

#include <QString>
#include <QStringList>
#include <string>

class QComboBox;

/** Shared SearchManager file-type combo and extension lists (SEGA / settings). */
namespace SearchFileTypes {

/** Built-in numbered types ("1"–"7") stored in settings; skip when listing custom types. */
inline bool isNumberedType(const std::string &type)
{
    return type.size() == 1 && type[0] >= '1' && type[0] <= '7';
}

/**
 * Fill combo with predefined + custom types and icons.
 * When forSearch is false, omit Directory/TTH (useless for local ext filters).
 * Each item stores SearchManager type id in Qt::UserRole (TYPE_LAST for custom).
 */
void fillCombo(QComboBox *combo, bool forSearch = true);

/** Icon + translated name + type id for one combo row. */
void addTypeItem(QComboBox *combo, int typeId);

/** File-list Adult Video combo id (not a hub search type). */
int adultVideoType();
bool isAdultVideoType(int typeIndex);

/**
 * Extensions for a SearchManager type index (uppercase, no leading dot).
 * Empty list means no extension filter (Any / Directory / TTH / unknown).
 * For custom types (index >= TYPE_LAST), pass the combo item text as typeName.
 * Adult Video uses the Video extension list.
 */
QStringList extensionsFor(int typeIndex, const QString &typeName = QString());

} // namespace SearchFileTypes
