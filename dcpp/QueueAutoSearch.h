/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "QueueItem.h"
#include "SearchManager.h"

namespace dcpp {

struct AutoSearchPick {
    QueueItem* item = nullptr;
    string query;
    SearchManager::TypeModes type = SearchManager::TYPE_TTH;
};

/** Background queue search: alternates TTH and full-filename picks,
 *  falling back to the other kind when the preferred one has no candidate. */
class QueueAutoSearch {
public:
    enum Mode { TTH, FILENAME };

    static AutoSearchPick pickAlternating(QueueItem::StringMap& queue, StringList& recent,
                                          StringList& recentNames, bool preferTTH);
};

} // namespace dcpp
