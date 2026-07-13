/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/** Alpha-numeric natural string order for download queue tie-breaks. */

#include "stdinc.h"
#include "NaturalCompare.h"

#include "Text.h"

#include <cstring>

namespace dcpp {

namespace {

bool isDigitByte(unsigned char c) {
    return c >= '0' && c <= '9';
}

int compareDigitRuns(const char*& a, const char*& b) {
    const char* za = a;
    while(*za == '0')
        ++za;
    const char* zb = b;
    while(*zb == '0')
        ++zb;

    const char* ea = za;
    while(isDigitByte(static_cast<unsigned char>(*ea)))
        ++ea;
    const char* eb = zb;
    while(isDigitByte(static_cast<unsigned char>(*eb)))
        ++eb;

    const size_t la = static_cast<size_t>(ea - za);
    const size_t lb = static_cast<size_t>(eb - zb);
    if(la != lb)
        return la < lb ? -1 : 1;

    const int cmp = std::strncmp(za, zb, la);
    if(cmp != 0)
        return cmp < 0 ? -1 : 1;

    // Same value: fewer leading zeros first ("1" before "01").
    const size_t totalA = static_cast<size_t>(ea - a);
    const size_t totalB = static_cast<size_t>(eb - b);
    a = ea;
    b = eb;
    if(totalA != totalB)
        return totalA < totalB ? -1 : 1;
    return 0;
}

} // namespace

int compareNatural(const string& a, const string& b) {
    const char* pa = a.c_str();
    const char* pb = b.c_str();

    while(*pa || *pb) {
        if(isDigitByte(static_cast<unsigned char>(*pa)) &&
           isDigitByte(static_cast<unsigned char>(*pb))) {
            const int digitCmp = compareDigitRuns(pa, pb);
            if(digitCmp != 0)
                return digitCmp;
            continue;
        }

        wchar_t ca = 0, cb = 0;
        const int na = Text::utf8ToWc(pa, ca);
        const int nb = Text::utf8ToWc(pb, cb);
        ca = Text::toLower(ca);
        cb = Text::toLower(cb);
        if(ca != cb)
            return static_cast<int>(ca) - static_cast<int>(cb);
        if(!*pa || !*pb)
            break;
        pa += abs(na);
        pb += abs(nb);
    }

    return 0;
}

} // namespace dcpp
