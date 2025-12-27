/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "screen.h"

class CreditsScreen : public Screen
{
public: 
    CreditsScreen();
    virtual ~CreditsScreen();

    void Draw();

    bool Update(VPADStatus status);
private:
    void ColourLoop();
};