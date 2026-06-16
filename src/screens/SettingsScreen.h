#pragma once

#include <SDL2/SDL.h>

namespace SettingsScreen {
    void initialize(SDL_Renderer *renderer);

    void finalize();

    void process_ui();
}