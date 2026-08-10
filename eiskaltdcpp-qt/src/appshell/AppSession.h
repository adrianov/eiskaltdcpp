/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

class EiskaltApp;

/**
 * Runs one Qt UI session: SessionBootstrap bring-up at normal process
 * priority, event loop (yield allowed only after bootstrap), tear-down.
 */
class AppSession {
public:
    explicit AppSession(EiskaltApp &app);
    int run();

private:
    EiskaltApp &app_;
};
