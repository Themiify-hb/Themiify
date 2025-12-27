/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <vpad/input.h>
#include <padscore/kpad.h>
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

// Copied straight from Wii U Ident lol
namespace {
uint32_t RemapWiiMoteButtons(uint32_t wiimoteButtons, uint32_t nunchukButtons)
{
    uint32_t conv_buttons = 0;

    if (wiimoteButtons & WPAD_BUTTON_LEFT)
        conv_buttons |= VPAD_BUTTON_LEFT;

    if (wiimoteButtons & WPAD_BUTTON_RIGHT)
        conv_buttons |= VPAD_BUTTON_RIGHT;

    if (wiimoteButtons & WPAD_BUTTON_DOWN)
        conv_buttons |= VPAD_BUTTON_DOWN;

    if (wiimoteButtons & WPAD_BUTTON_UP)
        conv_buttons |= VPAD_BUTTON_UP;

    if (wiimoteButtons & WPAD_BUTTON_PLUS)
        conv_buttons |= VPAD_BUTTON_PLUS;

    if (wiimoteButtons & WPAD_BUTTON_B)
        conv_buttons |= VPAD_BUTTON_B;

    if (wiimoteButtons & WPAD_BUTTON_A)
        conv_buttons |= VPAD_BUTTON_A;
    
    if (wiimoteButtons & WPAD_BUTTON_1)
        conv_buttons |= VPAD_BUTTON_X;

    if (wiimoteButtons & WPAD_BUTTON_2)
        conv_buttons |= VPAD_BUTTON_Y;

    if (wiimoteButtons & WPAD_BUTTON_MINUS)
        conv_buttons |= VPAD_BUTTON_MINUS;

    if (wiimoteButtons & WPAD_BUTTON_HOME)
        conv_buttons |= VPAD_BUTTON_HOME;

    if (nunchukButtons & WPAD_NUNCHUK_STICK_EMULATION_LEFT)
        conv_buttons |= VPAD_STICK_L_EMULATION_LEFT;
    
    if (nunchukButtons & WPAD_NUNCHUK_STICK_EMULATION_RIGHT)
        conv_buttons |= VPAD_STICK_L_EMULATION_RIGHT;

    if (nunchukButtons & WPAD_NUNCHUK_STICK_EMULATION_DOWN)
        conv_buttons |= VPAD_STICK_L_EMULATION_DOWN;

    if (nunchukButtons & WPAD_NUNCHUK_STICK_EMULATION_UP)
        conv_buttons |= VPAD_STICK_L_EMULATION_UP;

    return conv_buttons;
}

uint32_t RemapClassicButtons(uint32_t buttons)
{
    uint32_t conv_buttons = 0;

    if (buttons & WPAD_CLASSIC_BUTTON_LEFT)
        conv_buttons |= VPAD_BUTTON_LEFT;

    if (buttons & WPAD_CLASSIC_BUTTON_RIGHT)
        conv_buttons |= VPAD_BUTTON_RIGHT;

    if (buttons & WPAD_CLASSIC_BUTTON_DOWN)
        conv_buttons |= VPAD_BUTTON_DOWN;

    if (buttons & WPAD_CLASSIC_BUTTON_UP)
        conv_buttons |= VPAD_BUTTON_UP;

    if (buttons & WPAD_CLASSIC_BUTTON_PLUS)
        conv_buttons |= VPAD_BUTTON_PLUS;

    if (buttons & WPAD_CLASSIC_BUTTON_X)
        conv_buttons |= VPAD_BUTTON_X;

    if (buttons & WPAD_CLASSIC_BUTTON_Y)
        conv_buttons |= VPAD_BUTTON_Y;

    if (buttons & WPAD_CLASSIC_BUTTON_B)
        conv_buttons |= VPAD_BUTTON_B;

    if (buttons & WPAD_CLASSIC_BUTTON_A)
        conv_buttons |= VPAD_BUTTON_A;

    if (buttons & WPAD_CLASSIC_BUTTON_MINUS)
        conv_buttons |= VPAD_BUTTON_MINUS;

    if (buttons & WPAD_CLASSIC_BUTTON_HOME)
        conv_buttons |= VPAD_BUTTON_HOME;

    if (buttons & WPAD_CLASSIC_BUTTON_ZR)
        conv_buttons |= VPAD_BUTTON_ZR;

    if (buttons & WPAD_CLASSIC_BUTTON_ZL)
        conv_buttons |= VPAD_BUTTON_ZL;

    if (buttons & WPAD_CLASSIC_BUTTON_R)
        conv_buttons |= VPAD_BUTTON_R;

    if (buttons & WPAD_CLASSIC_BUTTON_L)
        conv_buttons |= VPAD_BUTTON_L;

    return conv_buttons;
}

uint32_t RemapProButtons(uint32_t buttons)
{
    uint32_t conv_buttons = 0;

    if (buttons & WPAD_PRO_BUTTON_LEFT)
        conv_buttons |= VPAD_BUTTON_LEFT;

    if (buttons & WPAD_PRO_BUTTON_RIGHT)
        conv_buttons |= VPAD_BUTTON_RIGHT;

    if (buttons & WPAD_PRO_BUTTON_DOWN)
        conv_buttons |= VPAD_BUTTON_DOWN;

    if (buttons & WPAD_PRO_BUTTON_UP)
        conv_buttons |= VPAD_BUTTON_UP;

    if (buttons & WPAD_PRO_STICK_L_EMULATION_LEFT)
        conv_buttons |= VPAD_STICK_L_EMULATION_LEFT;
    
    if (buttons & WPAD_PRO_STICK_L_EMULATION_RIGHT)
        conv_buttons |= VPAD_STICK_L_EMULATION_RIGHT;

    if (buttons & WPAD_PRO_STICK_L_EMULATION_DOWN)
        conv_buttons |= VPAD_STICK_L_EMULATION_DOWN;
    
    if (buttons & WPAD_PRO_STICK_L_EMULATION_UP)
        conv_buttons |= VPAD_STICK_L_EMULATION_UP;

    if (buttons & WPAD_PRO_BUTTON_PLUS)
        conv_buttons |= VPAD_BUTTON_PLUS;

    if (buttons & WPAD_PRO_BUTTON_X)
        conv_buttons |= VPAD_BUTTON_X;

    if (buttons & WPAD_PRO_BUTTON_Y)
        conv_buttons |= VPAD_BUTTON_Y;

    if (buttons & WPAD_PRO_BUTTON_B)
        conv_buttons |= VPAD_BUTTON_B;

    if (buttons & WPAD_PRO_BUTTON_A)
        conv_buttons |= VPAD_BUTTON_A;

    if (buttons & WPAD_PRO_BUTTON_MINUS)
        conv_buttons |= VPAD_BUTTON_MINUS;

    if (buttons & WPAD_PRO_BUTTON_HOME)
        conv_buttons |= VPAD_BUTTON_HOME;

    if (buttons & WPAD_PRO_BUTTON_ZR)
        conv_buttons |= VPAD_BUTTON_ZR;

    if (buttons & WPAD_PRO_BUTTON_ZL)
        conv_buttons |= VPAD_BUTTON_ZL;

    if (buttons & WPAD_PRO_BUTTON_R)
        conv_buttons |= VPAD_BUTTON_R;

    if (buttons & WPAD_PRO_BUTTON_L)
        conv_buttons |= VPAD_BUTTON_L;

    return conv_buttons;
}

void UpdatePads(VPADStatus* status)
{
    KPADStatus kpad_data{};
    KPADError kpad_error;
    for (int i = 0; i < 4; i++) {
        if (KPADReadEx((KPADChan) i, &kpad_data, 1, &kpad_error) > 0) {
            if (kpad_error == KPAD_ERROR_OK && kpad_data.extensionType != 0xFF) {
                if (kpad_data.extensionType == WPAD_EXT_CORE || kpad_data.extensionType == WPAD_EXT_NUNCHUK) {
                    status->trigger |= RemapWiiMoteButtons(kpad_data.trigger, kpad_data.nunchuk.trigger);
                    status->release |= RemapWiiMoteButtons(kpad_data.release, kpad_data.nunchuk.release);
                    status->hold |= RemapWiiMoteButtons(kpad_data.hold, kpad_data.nunchuk.hold);
                } else if (kpad_data.extensionType == WPAD_EXT_CLASSIC) {
                    status->trigger |= RemapClassicButtons(kpad_data.classic.trigger);
                    status->release |= RemapClassicButtons(kpad_data.classic.release);
                    status->hold |= RemapClassicButtons(kpad_data.classic.hold);
                } else if (kpad_data.extensionType == WPAD_EXT_PRO_CONTROLLER) {
                    status->trigger |= RemapProButtons(kpad_data.pro.trigger);
                    status->release |= RemapProButtons(kpad_data.pro.release);
                    status->hold |= RemapProButtons(kpad_data.pro.hold);
                }
            }
        }
    }
}
}

int main(int argc, char **argv)
{
    WHBProcInit();
    
    WHBLogUdpInit();
    WHBLogCafeInit();

    AXInit();

    KPADInit();
    WPADEnableURCC(TRUE);

    Gfx::Init();

    std::unique_ptr<Screen> mainScreen = std::make_unique<MainScreen>();

    while (WHBProcIsRunning()) {
        VPADStatus input{};
        VPADRead(VPAD_CHAN_0, &input, 1, nullptr);
        UpdatePads(&input);

        if (!mainScreen->Update(input)) {
            SYSLaunchMenu();
        }

        mainScreen->Draw();
        Gfx::Draw();
    }

    mainScreen.reset();

    Gfx::Shutdown();

    KPADShutdown();
    
    AXQuit();

    WHBLogUdpDeinit();
    WHBLogCafeDeinit();

    WHBProcShutdown();

    return 0;
}