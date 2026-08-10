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
 * Brings a Qt UI session from process start to a ready event loop, and tears
 * it down afterward. Paints the main window before HashIndex / share / queue
 * load so cold start is not a blank delay.
 */
class SessionBootstrap {
public:
    explicit SessionBootstrap(EiskaltApp &app);

    void bringUp();
    void tearDown();

private:
    void startCoreShell();
    void loadShareData();
    void startUi();
    void loadChrome();
    void createWindow();
    void startServices();
    void startDeferredServices();
    void showAndPaint();
    void stopUi();
    void stopCore();

    EiskaltApp &app_;
};
