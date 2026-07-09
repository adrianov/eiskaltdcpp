/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC bloom-filter GET/SND transfer handler.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "ShareManager.h"

#include <cmath>

namespace dcpp {

void AdcHub::handle(AdcCommand::GET, AdcCommand& c) noexcept {
    if(c.getParameters().size() < 5) {
        if(c.getParameters().size() > 0) {
            if(c.getParam(0) == "blom") {
                send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC,
                                "Too few parameters for blom", AdcCommand::TYPE_HUB));
            } else {
                send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
                                "Unknown transfer type", AdcCommand::TYPE_HUB));
            }
        } else {
            send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC,
                            "Too few parameters for GET", AdcCommand::TYPE_HUB));
        }
        return;
    }
    const string& type = c.getParam(0);
    string sk, sh;
    if(type == "blom" && c.getParam("BK", 4, sk) && c.getParam("BH", 4, sh))  {
        ByteVector v;
        size_t m = Util::toUInt32(c.getParam(3)) * 8;
        size_t k = Util::toUInt32(sk);
        size_t h = Util::toUInt32(sh);

        if(k > 8 || k < 1) {
            send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
                            "Unsupported k", AdcCommand::TYPE_HUB));
            return;
        }
        if(h > 64 || h < 1) {
            send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
                            "Unsupported h", AdcCommand::TYPE_HUB));
            return;
        }
        size_t n = ShareManager::getInstance()->getSharedFiles();

        // Ideal size for m is n * k / ln(2), but we allow some slack
        if(m > size_t(5 * Util::roundUp((int64_t)(n * k / log(2.)), (int64_t)64)) || m > static_cast<size_t>(1 << h)) {
            send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
                            "Unsupported m", AdcCommand::TYPE_HUB));
            return;
        }
        if (m > 0) {
            ShareManager::getInstance()->getBloom(v, k, m, h);
        }
        AdcCommand cmd(AdcCommand::CMD_SND, AdcCommand::TYPE_HUB);
        cmd.addParam(c.getParam(0));
        cmd.addParam(c.getParam(1));
        cmd.addParam(c.getParam(2));
        cmd.addParam(c.getParam(3));
        cmd.addParam(c.getParam(4));
        send(cmd);
        if (m > 0) {
            send((char*)&v[0], v.size());
        }
    }
}

} // namespace dcpp
