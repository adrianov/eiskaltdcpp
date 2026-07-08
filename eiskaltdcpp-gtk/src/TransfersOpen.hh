/*
 * Copyright © 2004-2010 Jens Oknelid, paskharen@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <dcpp/stdinc.h>

namespace TransfersOpen {

bool isOpenableTransfer(const std::string& filename);
std::string resolveTransferPath(bool download, const std::string& target, const std::string& tmpTarget, const std::string& filename);
std::string stripBracketedStatusPrefix(const std::string& text);

} // namespace TransfersOpen
