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

class MainScreen : public Screen
{
public:
    MainScreen();
    virtual ~MainScreen();

    void Draw();

    bool Update(VPADStatus status);
    
private:
    enum {
        STATE_INIT,
        STATE_MOUNT_MLC,
        STATE_CHECK_STYLEMIIU_EXISTS,
        STATE_LOAD_MENU,
        STATE_IN_MENU,
    } mState;

    bool mStateFailure;

    std::unique_ptr<Screen> mMenuScreen;

    bool StyleMiiUExists();
};