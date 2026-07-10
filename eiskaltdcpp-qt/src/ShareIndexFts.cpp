/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ShareIndex.h"

#ifdef USE_QT_SQLITE

/** DuckDB FTS is word/BM25, not trigram. Substring AND uses contains() on name_cf/path_cf. */
bool ShareIndex::ensureFts(duckdb::Connection &)
{
    return true;
}

#endif
