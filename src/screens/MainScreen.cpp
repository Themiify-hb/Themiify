/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <mocha/mocha.h>

#include "MainScreen.h"
#include "MenuScreen.h"
#include "gfx.h"
#include "utils.h"

MainScreen::MainScreen()
    :   mState(STATE_INIT),
        mStateFailure(false),
        mMenuScreen()
{    
    Gfx::SetBackgroundColour(BACKGROUND_COLOUR);
}

MainScreen::~MainScreen()
{
    Mocha_UnmountFS("storage_mlc");
    Mocha_DeInitLibrary();
}

void MainScreen::Draw()
{
    Gfx::Clear(Gfx::GetBackgroundColour());

    Gfx::Print(-4, -1, "Themiify - A Wii U Theme Installer - %s\n-------------------------------------------------------", THEMIIFY_VERSION);

    if (mMenuScreen) {
        mMenuScreen->Draw();
        return;
    }

    switch (mState) {
        case STATE_INIT:
            if (mStateFailure) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);
                Gfx::Print(-4, 2, "Failed to initialize libmocha!");
                Gfx::Print(-4, 17, "                           B/HOME - Quit");
                break;
            }
            
            Gfx::Print(-4, 2, "Initializing...");
            break;
        case STATE_MOUNT_MLC:
            if (mStateFailure) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);
                Gfx::Print(-4, 2, "Failed to mount mlc, themes can only be\ninstalled via the cache.");
                Gfx::Print(-4, 17, "            A - Continue                         B - Quit");
                break;
            }
            
            Gfx::Print(-4, 4, "Mounting mlc...");
            break;
        case STATE_CHECK_STYLEMIIU_EXISTS:
            if (mStateFailure) {
                Gfx::SetBackgroundColour(BACKGROUND_ERR_COLOUR);
                Gfx::Print(-4, 2, "StyleMiiU was not found on your system!\n\nPlease ensure you have the StyleMiiU Aroma Plugin\ndownloaded on your system, Themiify will not work without it!\n\nStyleMiiU can be found on the Homebrew Appstore and on Github at:\nhttps://github.com/Themiify-hb/StyleMiiU-Plugin");
                Gfx::Print(-4, 17, "                           B/HOME - Quit");
                break;
            }

            Gfx::Print(-4, 6, "Checking for StyleMiiU...");
            break;
        case STATE_LOAD_MENU:
            Gfx::SetBackgroundColour(BACKGROUND_COLOUR);
            Gfx::Print(-4, 8, "Loading menu...");
            break;
        case STATE_IN_MENU:
            break;
    }
}

bool MainScreen::Update(VPADStatus status)
{
    if (mMenuScreen) {
        if (!mMenuScreen->Update(status)) {
            // menu is exiting
            return false;
        }
        return true;
    }

    switch (mState) {
        MochaUtilsStatus res;
        
        case STATE_INIT:
            std::filesystem::create_directories(THEMIIFY_ROOT);

            if ((res = Mocha_InitLibrary()) != MOCHA_RESULT_SUCCESS) {
                mStateFailure = true;
            }

            if (mStateFailure) {
                if (status.trigger & VPAD_BUTTON_B) {
                    return false;
                }
            }
            else {
                mState = STATE_MOUNT_MLC;
            }

            break;
        case STATE_MOUNT_MLC:
            if ((res = Mocha_MountFS("storage_mlc", nullptr, "/vol/storage_mlc01")) != MOCHA_RESULT_SUCCESS) {
                mStateFailure = true;
            }

            if (mStateFailure) {
                if (status.trigger & VPAD_BUTTON_A) {
                    mState = STATE_CHECK_STYLEMIIU_EXISTS;
                }
                else if (status.trigger & VPAD_BUTTON_B) {
                    return false;
                }
            }
            else {
                mState = STATE_CHECK_STYLEMIIU_EXISTS;
            }            

            break;
        case STATE_CHECK_STYLEMIIU_EXISTS:
            if (!StyleMiiUExists()) {
                mStateFailure = true;
            }

            if (mStateFailure) {
                if (status.trigger & VPAD_BUTTON_B) {
                    return false;
                }
            }
            else {
                mState = STATE_LOAD_MENU;
            }

            break;
        case STATE_LOAD_MENU:
            mMenuScreen = std::make_unique<MenuScreen>();
            mState = STATE_IN_MENU;
            break;
        case STATE_IN_MENU:
            break;
    };

    return true;
}

bool MainScreen::StyleMiiUExists()
{
    char environmentPathBuffer[0x100];

    MochaUtilsStatus res;
    if ((res = Mocha_GetEnvironmentPath(environmentPathBuffer, sizeof(environmentPathBuffer))) != MOCHA_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to get environment path. Are you running on Aroma? Result: %s", Mocha_GetStatusStr(res));
    }

    std::string styleMiiUPath = std::string(environmentPathBuffer) + "/plugins/stylemiiu.wps";

    if (std::filesystem::exists(styleMiiUPath)) {
        return true;
    }

    return false;
}