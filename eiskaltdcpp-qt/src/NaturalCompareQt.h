/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QString>

#include "dcpp/NaturalCompare.h"

/** Same digit-aware path-depth + natural order as dcpp::compareNatural for Qt grids. */
inline int compareNaturalQ(const QString &a, const QString &b) {
    return dcpp::compareNatural(a.toStdString(), b.toStdString());
}
