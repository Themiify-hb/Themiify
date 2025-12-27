/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <vpad/input.h>
#include <sndcore2/core.h>
#include <sysapp/launch.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>

#include "gfx.h"
#include "screen.h"
#include "MainScreen.h"
#include "installer.h"
#include "utils.h"

int main(int argc, char **argv)
{
    WHBProcInit();
    
    WHBLogUdpInit();
    WHBLogCafeInit();

    AXInit(); // Perro made some music imma add soon enough

    VPADStatus status;

    Gfx::Init();

    std::unique_ptr<Screen> mainScreen = std::make_unique<MainScreen>();

    while (WHBProcIsRunning()) {
        VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

        if (!mainScreen->Update(status)) {
            SYSLaunchMenu();
        }

        mainScreen->Draw();
        Gfx::Draw();
    }

    mainScreen.reset();

    Gfx::Shutdown();

    AXQuit();

    WHBLogUdpDeinit();
    WHBLogCafeDeinit();

    WHBProcShutdown();

    return 0;
}