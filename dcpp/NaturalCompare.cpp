/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/** Alpha-numeric natural string order; path depth first when / or \\ present.
 *  Adjacent number+time-unit runs (h, min, s, ms, …) compare as elapsed time. */

#include "stdinc.h"
#include "NaturalCompare.h"

#include "Text.h"

#include <cstring>

namespace dcpp {

namespace {

bool isDigitByte(unsigned char c) {
    return c >= '0' && c <= '9';
}

/** Non-empty path segments when / or \\ present; else 0 (plain names keep natural order). */
size_t pathDepth(const string& s) {
    size_t depth = 0;
    bool inSeg = false;
    bool hasSep = false;
    for(char c: s) {
        if(c == '/' || c == '\\') {
            hasSep = true;
            inSeg = false;
        } else if(!inSeg) {
            inSeg = true;
            ++depth;
        }
    }
    return hasSep ? depth : 0;
}

bool asciiEq(const char* p, const char* w, size_t n) {
    for(size_t i = 0; i < n; ++i) {
        char c = p[i];
        if(c >= 'A' && c <= 'Z')
            c = static_cast<char>(c + 32);
        if(c != w[i])
            return false;
    }
    return true;
}

bool isAsciiLetter(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/** Time unit at p → milliseconds. Rejects prefixes of longer words ("s" in "song"). */
bool matchTimeUnit(const char* p, uint64_t& ms, size_t& len) {
    static const struct { const char* n; uint64_t ms; } units[] = {
        {"min", 60000}, {"ms", 1}, {"mn", 60000},
        {"h", 3600000ull}, {"s", 1000}, {"d", 86400000ull}
    };
    for(const auto& u: units) {
        const size_t l = std::strlen(u.n);
        if(!asciiEq(p, u.n, l) || isAsciiLetter(static_cast<unsigned char>(p[l])))
            continue;
        ms = u.ms;
        len = l;
        return true;
    }
    return false;
}

/** One or more "N unit" tokens (MediaInfo "1 h 56 min", "7 min 58 s") as milliseconds. */
bool parseDurationMs(const char*& p, uint64_t& out) {
    const char* cur = p;
    uint64_t total = 0;
    int parts = 0;
    for(;;) {
        const char* save = cur;
        while(*cur == ' ' || *cur == '\t')
            ++cur;
        if(!isDigitByte(static_cast<unsigned char>(*cur)))
            break;

        uint64_t n = 0;
        while(isDigitByte(static_cast<unsigned char>(*cur))) {
            n = n * 10 + static_cast<uint64_t>(*cur - '0');
            ++cur;
        }
        while(*cur == ' ' || *cur == '\t')
            ++cur;

        uint64_t unitMs = 0;
        size_t ulen = 0;
        if(!matchTimeUnit(cur, unitMs, ulen)) {
            cur = save;
            break;
        }
        cur += ulen;
        total += n * unitMs;
        ++parts;
    }
    if(parts == 0)
        return false;
    p = cur;
    out = total;
    return true;
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
    // Paths: fewer segments first (/a/b and a/b/ both depth 2); plain names skip this.
    const size_t depthA = pathDepth(a);
    const size_t depthB = pathDepth(b);
    if(depthA != depthB)
        return depthA < depthB ? -1 : 1;

    const char* pa = a.c_str();
    const char* pb = b.c_str();

    while(*pa || *pb) {
        if(isDigitByte(static_cast<unsigned char>(*pa)) &&
           isDigitByte(static_cast<unsigned char>(*pb))) {
            const char* da = pa;
            const char* db = pb;
            uint64_t ma = 0, mb = 0;
            if(parseDurationMs(da, ma) && parseDurationMs(db, mb)) {
                if(ma != mb)
                    return ma < mb ? -1 : 1;
                pa = da;
                pb = db;
                continue;
            }
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
