/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <SDL2/SDL.h>

namespace ManageThemesScreen {
    void initialize(SDL_Renderer *renderer);

    void finalize();

    void force_refresh();

    void process_ui();
}