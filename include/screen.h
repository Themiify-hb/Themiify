/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vpad/input.h>

class Screen
{
public:
    Screen() = default;
    virtual ~Screen() = default;

    virtual void Draw() = 0;

    virtual bool Update(VPADStatus status) = 0;
};