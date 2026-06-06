#pragma once

#include <SDL2/SDL.h>

#include "NavBar.h"

namespace ContentPanel {
    void initialize(SDL_Renderer *renderer);

    void finalize();

    void process_ui(NavBar::Tab tab);
}