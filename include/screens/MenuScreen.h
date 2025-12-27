/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include "screen.h"

class MenuScreen : public Screen
{
public:
    MenuScreen();
    virtual ~MenuScreen();

    void Draw();

    bool Update(VPADStatus status);

private:
    std::unique_ptr<Screen> mSubscreen;

    enum MenuID {
        MENU_ID_INSTALL_THEME,
        MENU_ID_INSTALLED_THEMES,
        MENU_ID_CREDITS,

        MENU_ID_MIN = MENU_ID_INSTALL_THEME,
        MENU_ID_MAX = MENU_ID_CREDITS,
    };

    MenuID mSelected;
};