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
 * Owns one Qt UI session: core bring-up, window/services, event loop, and
 * orderly teardown (including process-priority restore before exit).
 */
class AppSession {
public:
    explicit AppSession(EiskaltApp &app);
    int run();

private:
    void startCore();
    void startUi();
    void loadChrome();
    void createWindow();
    void startServices();
    void showWindow();
    void stopUi();
    void stopCore();

    EiskaltApp &app_;
};
