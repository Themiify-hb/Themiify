/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdint>

namespace Gfx {
    bool Init();

    void Shutdown();

    void Clear(uint32_t colour);

    void Draw();

    void Print(int x, int y, const char *fmt, ...);

    void SetBackgroundColour(uint32_t colour);

    uint32_t GetBackgroundColour();
}