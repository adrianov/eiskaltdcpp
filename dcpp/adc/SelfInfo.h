/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "AdcCommand.h"
#include "../typedefs.h"

namespace dcpp {

/**
 * Our ADC INF advertisement to a hub: diffs fields against the last broadcast
 * so only changed keys are sent (and empty values clear prior keys).
 */
class AdcSelfInfo {
public:
    struct Snap {
        string cid;
        string pid;
        string nick;
        string description;
        string slotCount;
        string freeSlots;
        string shareSize;
        string shareFiles;
        string email;
        string hubsNormal;
        string hubsReg;
        string hubsOp;
        string app;
        string version;
        string away;       // "1" or empty
        string locale;     // BCP47 LC
        string downLimit;  // bytes/s or empty
        string upLimit;    // bytes/s
        string keyprint;   // "SHA256/..." or empty
        string ip;         // I4 (0.0.0.0 = ask hub); empty when passive without NATT
        string udpPort;    // U4 when active
        bool active = false;
        bool allowNatt = false;
        bool tls = false;
        string sega;
        string adcs;
        string tcp4;
        string udp4;
        string nat0;
    };

    void clear() { last.clear(); }

    /** Write changed fields into c. */
    void write(AdcCommand& c, const Snap& s);

private:
    void put(AdcCommand& c, const string& key, const string& value);
    StringMap last;
};

} // namespace dcpp
