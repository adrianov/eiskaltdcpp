/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Smoke checks for per-hub SearchQueue scheduling.
 */

#include "stdinc.h"
#include "hub/SearchQueue.h"

#include <cstdio>

using namespace dcpp;

namespace {

bool expect(bool ok, const char* what, int& fails) {
    if(!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++fails;
    }
    return ok;
}

SearchCore makeSearch(const string& query, const string& token, void* owner) {
    SearchCore s;
    s.sizeType = 0;
    s.size = 0;
    s.fileType = 0;
    s.query = query;
    s.token = token;
    s.owners.insert(owner);
    return s;
}

} // namespace

int main() {
    int fails = 0;
    void* tabA = reinterpret_cast<void*>(1);
    void* tabB = reinterpret_cast<void*>(2);
    const uint64_t t0 = 100000;

    // Manual before auto; ETA by position * interval.
    {
        SearchQueue q;
        q.setInterval(60000);
        q.add(makeSearch("auto1", "auto", nullptr));
        q.add(makeSearch("man1", "tok1", tabA));
        q.add(makeSearch("auto2", "auto", nullptr));

        expect(q.getSearchTime(tabA, t0) == t0, "manual ETA at nextAllowed", fails);

        SearchCore out;
        expect(q.pop(out, t0) && out.query == "man1", "pop manual first", fails);
        expect(q.pop(out, t0) == false, "paced: second pop blocked", fails);
        expect(q.pop(out, t0 + 60000) && out.query == "auto1", "auto after interval", fails);
        expect(q.pop(out, t0 + 120000) && out.query == "auto2", "second auto", fails);
    }

    // Hub rate-limit raises floor and defers next send.
    {
        SearchQueue q;
        q.setInterval(10000);
        q.delayNext(64, t0);
        expect(q.getInterval() == 65000, "interval floor from hub secs+1", fails);
        q.add(makeSearch("q", "t", tabA));
        SearchCore out;
        expect(q.pop(out, t0 + 64000 - 1) == false, "before delay", fails);
        expect(q.pop(out, t0 + 64000) && out.query == "q", "after delay", fails);
    }

    // cancelSearch removes every item for that owner.
    {
        SearchQueue q;
        q.setInterval(1000);
        q.add(makeSearch("a", "t1", tabA));
        q.add(makeSearch("b", "t2", tabA));
        q.add(makeSearch("c", "t3", tabB));
        expect(q.cancelSearch(tabA), "cancel tabA", fails);
        expect(q.getSearchTime(tabA, t0) == 0, "tabA gone", fails);
        expect(q.getSearchTime(tabB, t0) == t0, "tabB still first", fails);
        SearchCore out;
        expect(q.pop(out, t0) && out.query == "c", "only tabB left", fails);
    }

    // Unpaced: drain without waiting.
    {
        SearchQueue q;
        q.setInterval(0);
        q.add(makeSearch("x", "t", tabA));
        q.add(makeSearch("y", "t2", tabB));
        SearchCore out;
        expect(q.pop(out, t0) && out.query == "x", "unpaced first", fails);
        expect(q.pop(out, t0) && out.query == "y", "unpaced second same tick", fails);
    }

    // Hub rate-limit raises floor; a later lower cfg reload must not wipe it.
    {
        SearchQueue q;
        q.setInterval(10000);
        q.delayNext(64, t0);
        expect(q.getInterval() == 65000, "hub floor raised", fails);
        q.raiseInterval((uint64_t)(10 + 1) * 1000);
        expect(q.getInterval() == 65000, "reload must not lower hub floor", fails);
    }

    if(fails) {
        std::fprintf(stderr, "%d search-queue smoke failure(s)\n", fails);
        return 1;
    }
    std::printf("search-queue smoke ok\n");
    return 0;
}
