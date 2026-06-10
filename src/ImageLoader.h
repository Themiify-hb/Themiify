/*
 * Themiify - A theme manager for the Nintendo Wii U
 * Copyright (C) 2026 Fangal-Airbag  
 * Copyright (C) 2026 AlphaCraft9658
 * Copyright (C) 2026  Daniel K. O. <dkosmari>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>

#include <SDL2/SDL.h>

namespace ImageLoader {
    void initialize(SDL_Renderer *renderer);

    void finalize();

    SDL_Texture *get(const std::string& location);
}