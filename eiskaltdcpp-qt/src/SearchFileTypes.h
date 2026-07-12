/***************************************************************************
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

class QComboBox;

/** Shared SearchManager file-type combo and extension lists (SEGA / settings). */
namespace SearchFileTypes {

/**
 * Fill combo with predefined + custom types and icons.
 * When forSearch is false, omit TTH (useless for local ext filters).
 * Each item stores SearchManager type id in Qt::UserRole (TYPE_LAST for custom).
 */
void fillCombo(QComboBox *combo, bool forSearch = true);

/**
 * Extensions for a SearchManager type index (uppercase, no leading dot).
 * Empty list means no extension filter (Any / Directory / TTH / unknown).
 * For custom types (index >= TYPE_LAST), pass the combo item text as typeName.
 */
QStringList extensionsFor(int typeIndex, const QString &typeName = QString());

} // namespace SearchFileTypes
