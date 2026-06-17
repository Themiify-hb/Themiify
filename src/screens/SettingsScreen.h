#pragma once

#include <SDL2/SDL.h>

namespace SettingsScreen {
    bool check_is_first_boot();
    void run_first_boot_check();
    void run_boot_integrity_check();
    
    void initialize(SDL_Renderer *renderer);

    void finalize();

    void process_ui();
}