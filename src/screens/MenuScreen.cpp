/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gfx.h"
#include "utils.h"

#include "MenuScreen.h"
#include "InstallThemeScreen.h"
#include "InstalledThemesScreen.h"
#include "CreditsScreen.h"

MenuScreen::MenuScreen()
    :   mSubscreen(),
        mSelected(MENU_ID_MIN)
{
}

MenuScreen::~MenuScreen()
{
}

void MenuScreen::Draw()
{
    if (mSubscreen) {
        mSubscreen->Draw();
        return;
    }

    Gfx::SetBackgroundColour(BACKGROUND_COLOUR);
    
    Gfx::Print(-4, 2, (mSelected == MENU_ID_INSTALL_THEME) ? "> Install Theme" : "  Install Theme");
    Gfx::Print(-4, 3, (mSelected == MENU_ID_INSTALLED_THEMES) ? "> Installed Themes" : "  Installed Themes");
    Gfx::Print(-4, 4, (mSelected == MENU_ID_CREDITS) ? "> Credits" : "  Credits");

    Gfx::Print(-4, 17, "A - Select               D-Pad/Stick - Move           HOME/B - Quit");
}

bool MenuScreen::Update(VPADStatus status)
{
    if (mSubscreen) {
        if (!mSubscreen->Update(status)) {
            mSubscreen.reset();
        }
        return true;
    }

    if (status.trigger & (VPAD_STICK_L_EMULATION_DOWN | VPAD_BUTTON_DOWN)) {
        if (mSelected < MENU_ID_MAX) {
            mSelected = static_cast<MenuID>(mSelected + 1);
        }
    }
    else if (status.trigger & (VPAD_STICK_L_EMULATION_UP | VPAD_BUTTON_UP)) {
        if (mSelected > MENU_ID_MIN) {
            mSelected = static_cast<MenuID>(mSelected - 1);
        }
    }

    if (status.trigger & VPAD_BUTTON_A) {
        switch (mSelected) {
            case MENU_ID_INSTALL_THEME:
                mSubscreen = std::make_unique<InstallThemeScreen>();
                break;
            case MENU_ID_INSTALLED_THEMES:
                mSubscreen = std::make_unique<InstalledThemesScreen>();
                break;
            case MENU_ID_CREDITS:
                mSubscreen = std::make_unique<CreditsScreen>();
                break;
        };
    }

    if (status.trigger & VPAD_BUTTON_B) {
        return false;
    }

    return true;
}