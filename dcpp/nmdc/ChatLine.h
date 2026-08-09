/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../typedefs.h"

namespace dcpp {

class NmdcHub;

/**
 * Non-command NMDC hub text: public chat and free-form status lines.
 * Owns the parse path (nick/body/spam filters) for one hub session.
 */
class NmdcChatLine {
public:
    explicit NmdcChatLine(NmdcHub& hub) : hub(hub) {}

    /** Consume a UTF-8 hub line that does not start with '$'. */
    void handle(const string& utf8Line);

private:
    NmdcHub& hub;

    void status(const string& text, int flags = 0);
    /** Ban watch + search/connect limit text from hub notices. */
    void honorLimits(const string& text);
    void chat(const string& nick, const string& message);
    static bool mentionsBan(const string& text);
};

} // namespace dcpp
