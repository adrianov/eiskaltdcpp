/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#ifdef USE_QT_SQLITE

#include <string>

namespace dcpp {
class HintedUser;
}

namespace ReciprocalListPeer {

bool cooldownActive(const dcpp::HintedUser &peer);
void markChecked(const dcpp::HintedUser &peer);
/** True when peer or a same-list alias has a reusable cache; sets listFile to peer's path. */
bool findCachedList(const dcpp::HintedUser &peer, std::string &listFile);

} // namespace ReciprocalListPeer

#endif
